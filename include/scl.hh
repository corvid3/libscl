#pragma once

/* generated by c++gen */

// TODO: `value` has some really really lax constraints
// for conversion and construction
// should probably make a "hypervalue" structure
// that has all of the constraints & is trivially castable
// as to hide away the behavior to not be default

#include <concepts>
#include <exception>
#include <format>
#include <functional>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace scl {

class value;

using string = std::string;
using number = double;
using boolean = bool;
class array;

using table = std::map<std::string, value, std::less<>>;
using table_array = std::list<table>;

template<typename T>
concept numeric =
  requires { requires std::integral<T> or std::floating_point<T>; };

template<typename T>
concept is_enum = requires { requires std::is_enum_v<T>; };

class _type_consolidator
{
  friend class _deser_impl;
  friend class _ser_impl;
  friend class value;
  friend class array;

  static scl::number consol(numeric auto const);
  static scl::string consol(std::convertible_to<scl::string> auto const);
  static scl::boolean consol(bool const);
  static scl::array consol(array const);
  static scl::string consol(is_enum auto const);

  template<typename... Ts>
  static std::tuple<Ts...> consol(std::tuple<Ts...>);

  template<typename T>
  static scl::array consol(std::vector<T>);

  template<typename T>
  static decltype(consol(std::declval<T>())) consol(std::optional<T>);

  template<typename T>
  using get = decltype(consol(std::declval<T>()));
};

class array : public std::vector<value>
{
  // yes, deriving from a standard library type
  // so scary!
  // not attaching anything to it outside of some implicit conversion functions
  // though
public:
  template<typename T>
  operator std::vector<T>();

  using vector::vector;

  template<typename T>
  array(std::vector<T> const& from);
};

struct typename_visitor
{
  constexpr std::string_view operator()(std::monostate const&)
  {
    return "uninitialized";
  }

  constexpr std::string_view operator()(string const&) { return "string"; }
  constexpr std::string_view operator()(numeric auto const) { return "number"; }
  constexpr std::string_view operator()(array const&) { return "array"; }
  constexpr std::string_view operator()(bool const&) { return "boolean"; }
  constexpr std::string_view operator()(is_enum auto const) { return "enum"; }

  template<typename T>
  constexpr std::string_view operator()(std::optional<T> const&)
  {
    return "optional";
  }

  template<typename T>
  constexpr std::string_view operator()(std::vector<T> const&)
  {
    return "type-set array";
  }

  template<typename... T>
  constexpr std::string_view operator()(std::tuple<T...>)
  {
    static std::string what;
    static std::once_flag call_once;
    std::call_once(
      call_once,
      [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        std::stringstream gen;
        gen << std::format("constrained array/tuple: len {} | ", sizeof...(T));
        ((gen << std::format("arg {}: {}, ", Is, typename_visitor()(T()))),
         ...);
        what = gen.str();
      },
      std::index_sequence_for<T...>());
    return what;
  }
};

// thrown when an array->tuple deserialization/get
// fails due to the array having the wrong size
class wrong_size_exception : public std::runtime_error
{
public:
  constexpr wrong_size_exception(auto const expected_size,
                                 auto const found_size)
    : std::runtime_error(std::format("failed to convert a scl array to a "
                                     "tuple, expected {} but found size of {}",
                                     expected_size,
                                     found_size)) {};
};

// defer the runtime error report definition
// instead of constantly generating them
// every time this cast is called
template<typename... Ts>
struct tuple_cast_exception : std::runtime_error
{
  template<std::size_t... Is>
  static std::string generate(std::index_sequence<Is...>)
  {
    std::stringstream s;

    s << "error when attempting to cast a scl::value to a tuple: ";
    s << std::format("expected array size: {} | ", sizeof...(Ts));
    ((s << std::format("arg {}: {}, ", Is, typename_visitor()(Ts()))), ...);

    return s.str();
  };

  tuple_cast_exception()
    : std::runtime_error(generate(std::index_sequence_for<Ts...>())) {};
};

class value
{
  template<typename T, typename EXCEPTION_TYPE>
  struct get_impl;

  // for tuple types
  template<typename... T, typename EXCEPTION_TYPE>
  struct get_impl<std::tuple<T...>, EXCEPTION_TYPE>
  {
    template<std::size_t... I>
    constexpr void handle(auto const& arr,
                          std::tuple<T...>& tup,
                          std::index_sequence<I...>,
                          auto const... to_throw) const
    {
      ((std::get<I>(tup) =
          arr.at(I).template get<T, EXCEPTION_TYPE>(to_throw...)),
       ...);
    }

    constexpr auto operator()(value const& v, auto const... to_throw) const
    {
      constexpr auto expected_size = sizeof...(T);

      if (std::holds_alternative<scl::array>(v.m_value)) {
        auto const& arr = std::get<scl::array>(v.m_value);

        if (arr.size() != expected_size)
          throw wrong_size_exception(expected_size, arr.size());

        // for each T, go to each index in the array & call get,
        // then push them into a tuple and return the tuple
        using index_seq = std::index_sequence_for<T...>;
        std::tuple<T...> out;

        handle(arr, out, index_seq(), to_throw...);

        return out;
      } else
        throw EXCEPTION_TYPE(to_throw...);
    }
  };

  // for everything but tuple types
  template<typename T, typename EXCEPTION_TYPE>
  struct get_impl
  {
    constexpr auto operator()(value const& v, auto const... to_throw) const
    {
      if (std::holds_alternative<T>(v.m_value))
        return std::get<T>(v.m_value);
      else
        throw EXCEPTION_TYPE(to_throw...);

      std::unreachable();
    }

    constexpr auto& operator()(value const& v, auto const... to_throw)
    {
      if (std::holds_alternative<T>(v.m_value))
        return std::get<T>(v.m_value);
      else
        throw EXCEPTION_TYPE(to_throw...);

      std::unreachable();
    }
  };

public:
  struct init_empty_m
  {};

  constexpr value(init_empty_m)
    : m_value(std::monostate()) {};

  constexpr value(std::string_view const& str)
    : m_value(std::string(str)) {};
  constexpr value(std::convertible_to<double> auto const num)
    : m_value(double(num)) {};
  constexpr value(bool const b)
    : m_value(b) {};
  constexpr value(array&& a)
    : m_value(a) {};

  template<typename... Ts>
  constexpr value(std::tuple<Ts...> const& data)
  {
    array arr;
    std::apply(
      [&](Ts const&... v) {
        (arr.push_back(value(v)), ...);
        ;
      },
      data);
    m_value = std::move(arr);
  }

  constexpr value(value const& rhs)
    : m_value(rhs.m_value) {};

  constexpr value& operator=(value const& rhs)
  {
    m_value = rhs.m_value;
    return *this;
  }

  template<typename... Ts>
  constexpr operator std::tuple<Ts...>() const
  {

    return this->get<std::tuple<Ts...>, tuple_cast_exception<Ts...>>();
  }

  template<typename T, typename EXCEPTION_TYPE = std::runtime_error>
  constexpr auto get(auto const... to_throw) const
  {
    using casted_type = _type_consolidator::get<T>;
    return get_impl<casted_type, EXCEPTION_TYPE>()(*this, to_throw...);
  }

  template<typename T, typename EXCEPTION_TYPE = std::runtime_error>
  constexpr auto& get(auto const... to_throw)
  {
    using casted_type = _type_consolidator::get<T>;
    return get_impl<casted_type, EXCEPTION_TYPE>()(*this, to_throw...);
  }

  constexpr void emplace(auto&& in) { m_value = in; }

  constexpr std::string_view get_internal_type_name() const
  {

    return std::visit(typename_visitor(), m_value);
  }

  template<typename T>
  constexpr bool holds() const
  {
    return std::holds_alternative<T>(m_value);
  }

  std::string constexpr serialize() const;

private:
  value() = default;

  std::variant<std::monostate, string, number, array, bool> m_value;
};

template<typename EXPECTED>
class vector_cast_exception final : std::runtime_error
{
public:
  vector_cast_exception(value const& found)
    : runtime_error(std::format("error when attempting to cast array to a "
                                "vector, expected <{}> but found <{}>",
                                typename_visitor()(EXPECTED()),
                                found.get_internal_type_name())) {};
};

// alllll the wayy down here...
template<typename T>
array::operator std::vector<T>()
{
  std::vector<T> out;

  for (auto const& v : *this)
    out.push_back(v.template get<T, vector_cast_exception<T>>(v));

  return out;
};

template<typename T>
array::array(std::vector<T> const& from)
{
  for (auto const& v : from)
    this->push_back(value(v));
}

struct value_serialize_visitor
{
  // comedically unsafe, doesn't escape anything
  constexpr std::string operator()(string const& s)
  {
    return std::format("\"{}\"", s);
  }

  constexpr std::string operator()(std::monostate const&)
  {
    throw std::runtime_error("monostate in value serialize?");
  }

  constexpr std::string operator()(array const& a)
  {
    std::string out;
    out += "{";

    for (auto const& v : a)
      out += " " + v.serialize() + " ";

    out += "}";
    return out;
  }

  constexpr std::string operator()(number const& n)
  {
    return std::format("{}", n);
  }
};

std::string constexpr value::serialize() const
{
  return std::visit(value_serialize_visitor(), m_value);
}

class scl_search_exception : public std::exception
{
public:
  scl_search_exception(std::string_view name)
    : name(name)
  {
    m_what = std::format("unable to find table/array <{}> in scl file", name);
  }

  // what name one is looking for
  std::string_view const name;

  char const* what() const noexcept override { return m_what.c_str(); }

private:
  std::string m_what;
};

class file
{
public:
  friend struct State;

  file() = default;

  // parses a scl_file
  file(std::string_view);

  bool table_exists(std::string_view name) const
  {
    return not(m_tables.find(name) == m_tables.end());
  }

  bool array_table_exists(std::string_view name) const
  {
    return not(m_tableArrays.find(name) == m_tableArrays.end());
  }

  auto& get_table(std::string_view name)
  {
    auto out = m_tables.find(name);
    if (out == m_tables.end())
      throw scl_search_exception(name);
    return out->second;
  }

  auto& get_table_array(std::string_view name)
  {
    auto out = m_tableArrays.find(name);
    if (out == m_tableArrays.end())
      throw scl_search_exception(name);
    return out->second;
  }

  auto const& get_table(std::string_view name) const
  {
    auto out = m_tables.find(name);
    if (out == m_tables.end())
      throw scl_search_exception(name);
    return out->second;
  }

  auto const& get_table_array(std::string_view name) const
  {
    auto out = m_tableArrays.find(name);
    if (out == m_tableArrays.end())
      throw scl_search_exception(name);
    return out->second;
  }

  auto num_tables() const { return m_tables.size(); }

  void insert_table(std::string_view name, table&& t) &
  {
    m_tables.insert_or_assign(std::string(name), t);
  }

  void insert_table_array(std::string_view name, table_array&& ta) &
  {
    m_tableArrays.insert_or_assign(std::string(name), ta);
  }

  std::string serialize() const
  {
    std::stringstream out;

    out << "# autogenerated by libscl\n";

    for (auto const& [k, t] : m_tables) {
      out << std::format("[{}]\n", k);
      for (auto const& [k, v] : t)
        out << std::format("{} = {}\n", k, v.serialize());
    }

    for (auto const& [k, ta] : m_tableArrays) {
      for (auto const& t : ta) {
        out << std::format("[[{}]]\n", k);
        for (auto const& [k, v] : t)
          out << std::format("{} = {}\n", k, v.serialize());
      }
    }

    return out.str();
  };

private:
  std::map<std::string, table, std::less<>> m_tables;
  std::map<std::string, table_array, std::less<>> m_tableArrays;
};

template<typename T>
concept is_vec = requires(T t) {
  { std::vector{ t } } -> std::same_as<T>;
};

template<typename T>
struct member_pointer_destructure;

template<typename CLASS, typename X>
struct member_pointer_destructure<X CLASS::*>
{
  using class_type = CLASS;
  using value_type = X;
};

template<typename CLASS, typename X>
struct member_pointer_destructure<X CLASS::* const>
{
  using class_type = CLASS;
  using value_type = X;
};

template<typename T, typename OF>
concept is_member_pointer_of = requires(T t) {
  requires std::same_as<typename member_pointer_destructure<T>::class_type, OF>;
};

template<std::size_t N>
struct field_name_literal
{
  constexpr field_name_literal(char const (&str)[N])
  {
    std::copy(str, str + N - 1, m_str.begin());
  }

  constexpr operator std::string_view() const
  {
    return { m_str.begin(), m_str.end() };
  }

  std::array<char, N - 1> m_str{};
};

template<field_name_literal a>
constexpr auto
operator""_f()
{
  return a;
};

template<typename T>
  requires std::is_enum_v<T>
using enum_deserialization_function = std::optional<T>(*)(std::string_view);

template<typename T>
  requires std::is_enum_v<T>
using enum_serialize_function = std::string_view(*)(T);

template<typename T>
using X = T;

template<auto const FIELD_PTR,
         field_name_literal const NAME,
         bool const MUST_EXIST = true>
struct field
{
  constexpr static auto ptr = FIELD_PTR;
  constexpr static std::string_view name = NAME;
  constexpr static bool must_exist = MUST_EXIST;
};

template<typename T,
         enum_deserialization_function<T> const ENUM_DESERIALIZE_FUNCTION,
         enum_serialize_function<T> const ENUM_SERIALIZE_FUNCTION>
struct enum_field_descriptor
{
  constexpr static enum_deserialization_function<T> deserialize_function =
    ENUM_DESERIALIZE_FUNCTION;

  constexpr static enum_serialize_function<T> serialize_function =
    ENUM_SERIALIZE_FUNCTION;
};

template<auto const FIELD_PTR,
         field_name_literal const NAME,
         typename ENUM_DESCRIPTOR,
         bool const MUST_EXIST = true>
struct enum_field
{
  constexpr static auto ptr = FIELD_PTR;
  constexpr static std::string_view name = NAME;
  constexpr static bool must_exist = MUST_EXIST;
  using descriptor = ENUM_DESCRIPTOR;
};

enum _empty_enum
{
};

template<typename T>
concept has_scl_fields_descriptor = requires { typename T::scl_fields; };
template<typename T>
concept has_scl_recurse_descriptor = requires { typename T::scl_recurse; };

// thrown when a table by a name doesn't exist
class deserialize_table_error final : public std::runtime_error
{
public:
  deserialize_table_error(std::string_view table_name)
    : std::runtime_error(
        std::format("expected a table of name <{}> when deserializing",
                    table_name))
    , m_tableName(table_name) {};

  std::string m_tableName;
};

// thrown when a field is not found and there is no
// default value for the field
class deserialize_field_error final : public std::runtime_error
{
public:
  deserialize_field_error(std::string_view name, std::string_view table_name)
    : std::runtime_error(std::format(
        "expected a nonexisteent field by name of <{}> in table <{}>",
        name,
        table_name))
    , m_fieldName(name)
    , m_tableName(table_name) {};

  std::string m_fieldName, m_tableName;
};

// thrown when a field is found in a table,
// but the field is the wrong type
class deserialize_field_type_error final : public std::runtime_error
{
public:
  deserialize_field_type_error(std::string_view name,
                               std::string_view table_name,
                               std::string_view expected_type_name,
                               std::string_view found_type_name)
    : std::runtime_error(
        std::format("failed to get table value by name of <{}>, expected "
                    "<{}> but found <{}>"
                    ", in table <{}>",
                    name,
                    expected_type_name,
                    found_type_name,
                    table_name))
    , m_fieldName(name)
    , m_tableName(table_name)
    , m_expectedTypename(expected_type_name)
    , m_foundTypename(found_type_name) {};

  std::string m_fieldName, m_tableName, m_expectedTypename, m_foundTypename;
};

template<typename field_type>
concept is_enum_field = requires(field_type F) {
  { enum_field{ F } } -> std::same_as<field_type>;
};

// thrown when attempting to deserialize a string into an enum
// but the string fails the conversion function
struct scl_enum_deserialize_error : public std::runtime_error
{
  scl_enum_deserialize_error(std::string_view table_name,
                             std::string_view key_name,
                             std::string_view what)
    : runtime_error(
        std::format("unknown enum value when trying to deserialize "
                    "scl file, in table <{}>, value <{}>, found <{}>",
                    table_name,
                    key_name,
                    what))
    , table_name(table_name)
    , key_name(key_name)
    , what(what)
  {
  }

  std::string table_name;
  std::string key_name;
  std::string what;
};

// implementation details for class deserialization
class _deser_impl
{

  template<typename FIELD, typename T>
    requires is_enum_field<FIELD>
  static void apply_conversion(std::string_view table_name,
                               std::string_view key_name,
                               T& into,
                               auto&& from)
  {
    auto const e = FIELD::descriptor::deserialize_function(from);

    if (e.has_value())
      into = *e;
    else
      throw scl_enum_deserialize_error(table_name, key_name, from);
  }

  template<typename FIELD, typename T>
    requires(not is_enum_field<FIELD>)
  static void apply_conversion(std::string_view,
                               std::string_view,
                               T& into,
                               auto&& from)
  {
    into = T(from);
  }

  template<typename fields>
  static void internal_deserialize_field_unwrapper(auto& into,
                                                   auto& table,
                                                   auto const& table_name)
  {
    std::apply(
      [&](auto const... FIELDS) {
        (
          [&]<typename FIELD>(FIELD) {
            auto constexpr field_ptr = FIELD::ptr;
            using field_type =
              member_pointer_destructure<decltype(field_ptr)>::value_type;
            using grab_type = _type_consolidator::get<field_type>;
            auto constexpr field_name = FIELD::name;
            auto constexpr field_must_parse = FIELD::must_exist;

            auto&& val = table.find(field_name);

            if (val == table.end()) {
              if (field_must_parse)
                throw deserialize_field_error(field_name, table_name);

            } else {
              // TODO: give better type error reporting by giving the name of
              // the type

              auto&& val2 =
                val->second
                  .template get<grab_type, deserialize_field_type_error>(
                    field_name,
                    table_name,
                    typename_visitor()(field_type()),
                    val->second.get_internal_type_name());

              apply_conversion<FIELD>(
                table_name, field_name, into.*field_ptr, std::move(val2));
            }
          }(FIELDS),
          ...);
      },
      fields());
  };

  template<typename recurses>
  static void internal_deserialize_recurse_wrapper(auto& into,
                                                   file const& file,
                                                   auto const& table_name)
  {
    std::apply(
      [&](auto const... RECURSES) {
        (
          [&]<typename RECURSE>(RECURSE) {
            auto constexpr field_ptr = RECURSE::ptr;
            // using field_type =
            //   member_pointer_destructure<decltype(field_ptr)>::value_type;
            auto field_name = std::format("{}.{}", table_name, RECURSE::name);
            auto constexpr must_exist = RECURSE::must_exist;
            using field_type =
              member_pointer_destructure<decltype(RECURSE::ptr)>::value_type;

            std::cout << std::format("wtf: {}\n", must_exist);

            auto& inner = into.*field_ptr;

            if constexpr (is_vec<field_type>)
              deserialize(
                std::inserter(inner, inner.begin()), file, field_name);
            else
              deserialize<std::decay_t<decltype(inner)>, false>(
                inner, file, field_name);
          }(RECURSES),
          ...);
      },
      recurses());
  }

  template<typename T>
    requires has_scl_recurse_descriptor<T>
  static void inner_deserialize_recurse(T& into,
                                        file const& file,
                                        auto const& table_name)
  {
    using recurses = T::scl_recurse;
    internal_deserialize_recurse_wrapper<recurses>(into, file, table_name);
  }

  template<typename T>
    requires(not has_scl_recurse_descriptor<T>)
  static void inner_deserialize_recurse(auto&, file const&, auto const&)
  {
  }

  // works on spans (array tables)
  template<typename T, bool err_on_no_table = true>
    requires has_scl_fields_descriptor<typename T::value_type>
  static bool deserialize(std::insert_iterator<T> into_iter,
                          file const& file,
                          std::string_view table_name)
  {
    using deser_type = T::value_type;
    using fields = deser_type::scl_fields;

    if (not file.array_table_exists(table_name)) {
      if (not err_on_no_table)
        return false;
      else
        throw deserialize_table_error(table_name);
    }

    auto const& array_table = file.get_table_array(table_name);

    for (auto const& table : array_table) {
      deser_type into;
      internal_deserialize_field_unwrapper<fields>(into, table, table_name);
      inner_deserialize_recurse<T>(into, file, table_name);
      into_iter = std::move(into);
    }

    return true;
  }

  // returns false if table doesn't exist
  template<has_scl_fields_descriptor T, bool err_on_no_table = true>
  static bool deserialize(T& into,
                          file const& file,
                          std::string_view table_name)
  {
    using fields = T::scl_fields;

    if (not file.table_exists(table_name)) {
      if (err_on_no_table)
        throw deserialize_table_error(table_name);
      else
        return false;
    }

    auto const& table = file.get_table(table_name);

    internal_deserialize_field_unwrapper<fields>(into, table, table_name);
    inner_deserialize_recurse<T>(into, file, table_name);

    return true;
  }

  template<typename T>
  static void deserialize(T& into, file const& file)
  {
    using recurses = T::scl_recurse;

    std::apply(
      [&]<typename... Rs>(Rs... rs) {
        (
          [&]<typename R>(R) {
            if constexpr (not is_vec<typename member_pointer_destructure<
                            decltype(R::ptr)>::value_type>)
              deserialize<typename member_pointer_destructure<
                            decltype(R::ptr)>::value_type,
                          R::must_exist>(into.*R::ptr, file, R::name);
            else
              deserialize<typename member_pointer_destructure<
                            decltype(R::ptr)>::value_type,
                          R::must_exist>(
                std::inserter(into.*R::ptr, (into.*R::ptr).end()),
                file,
                R::name);
          }(rs),
          ...);
      },
      recurses());
  }

  friend bool deserialize(auto&,
                          file const&,
                          std::convertible_to<std::string_view> auto);

  friend void deserialize(auto&, file const&);
};

// implementation details for the serialization interface
class _ser_impl
{
  template<typename recurses>
  static void internal_serialize_recurse_wrapper(auto const& into,
                                                 file& file,
                                                 auto const& table_name)
  {
    std::apply(
      [&](auto const... RECURSES) {
        (
          [&]<typename RECURSE>(RECURSE) {
            auto constexpr field_ptr = RECURSE::ptr;
            // using field_type =
            //   member_pointer_destructure<decltype(field_ptr)>::value_type;
            auto field_name = std::format("{}.{}", table_name, RECURSE::name);
            using field_type =
              member_pointer_destructure<decltype(RECURSE::ptr)>::value_type;

            auto& inner = into.*field_ptr;

            if constexpr (is_vec<field_type>)
              serialize<typename field_type::value_type>(
                std::span{ inner.begin(), inner.end() }, file, field_name);
            else
              serialize(inner, file, field_name);
          }(RECURSES),
          ...);
      },
      recurses());
  }

  template<typename T>
    requires has_scl_recurse_descriptor<T>
  static void inner_serialize_recurse(T const& into,
                                      file& file,
                                      auto const& table_name)
  {
    using recurses = T::scl_recurse;
    internal_serialize_recurse_wrapper<recurses>(into, file, table_name);
  }

  template<typename T>
    requires(not has_scl_recurse_descriptor<T>)
  static void inner_serialize_recurse(T const&, file&, auto const&)
  {
  }

  template<typename FIELD, typename T>
    requires is_enum_field<FIELD>
  static value apply_conversion(auto const& from)
  {
    return FIELD::descriptor::serialize_function(from);
  }

  template<typename FIELD, typename T>
    requires(not is_enum_field<FIELD>)
  static value apply_conversion(auto const& from)
  {
    return value(T(from));
  }

  template<typename FIELD, typename cast_to, typename read_type>
  static void optional_jank(auto& table,
                            std::string_view field_name,
                            std::optional<read_type> const& field)
  {
    if (field)
      table.insert_or_assign(std::string(field_name),
                             value(apply_conversion<FIELD, cast_to>(*field)));
  }

  template<typename FIELD, typename cast_to, typename read_type>
  static void optional_jank(auto& table,
                            std::string_view field_name,
                            read_type const& field)
  {
    table.insert_or_assign(std::string(field_name),
                           value(apply_conversion<FIELD, cast_to>(field)));
  }

  template<typename fields>
  static void inner_serialize_fields(auto const& from, auto& table)
  {
    std::apply(
      [&](auto const... FIELDS) {
        (
          [&]<typename FIELD>(FIELD) {
            auto constexpr field_ptr = FIELD::ptr;
            auto constexpr field_name = FIELD::name;
            using cast_to =
              _type_consolidator::get<typename member_pointer_destructure<
                decltype(field_ptr)>::value_type>;

            optional_jank<FIELD, cast_to>(table, field_name, from.*field_ptr);
          }(FIELDS),
          ...);
      },
      fields());
  }

  template<has_scl_fields_descriptor T>
  static void serialize(std::span<T const> from,
                        file& file,
                        std::string_view table_name)
  {
    using fields = T::scl_fields;

    table_array array;

    for (auto const& val : from) {
      table table_;

      inner_serialize_fields<fields>(val, table_);
      inner_serialize_recurse<T>(val, file, table_name);

      array.push_back(std::move(table_));
    }

    file.insert_table_array(table_name, std::move(array));
  };

  // replaces table if it already exists in the file
  template<has_scl_fields_descriptor T>
  static void serialize(T const& from, file& file, std::string_view table_name)
  {
    using fields = T::scl_fields;

    table table_;

    inner_serialize_fields<fields>(from, table_);
    inner_serialize_recurse<T>(from, file, table_name);

    file.insert_table(table_name, std::move(table_));
  };

  template<typename T>
  static void serialize(T const& into, file& file)
  {
    using recurses = T::scl_recurse;

    std::apply([&]<typename... R>(
                 R...) { (serialize(into.*R::ptr, file, R::name), ...); },
               recurses());
  }

  friend void serialize(auto const&, file&, std::string_view);
  friend void serialize(auto const&, file&);
};

// returns false if table doesn't exist
bool
deserialize(auto& into,
            file const& file,
            std::convertible_to<std::string_view> auto table_name)
{
  return _deser_impl::deserialize(into, file, table_name);
}

// used for "top level" table structures
void
deserialize(auto& into, file const& file)
{
  _deser_impl::deserialize(into, file);
}

void
serialize(auto const& from, file& into, std::string_view table_name)
{
  _ser_impl::serialize(from, into, table_name);
}

// used for "top level" table structures
void
serialize(auto const& from, file& into)
{
  _ser_impl::serialize(from, into);
}
};
