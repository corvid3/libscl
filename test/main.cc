#include "../include/scl.hh"
#include <format>
#include <fstream>
#include <iostream>
#include <optional>
#include <regex>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

struct inner_t
{};

enum class Enum
{
  Executable,
};

std::optional<Enum>
enum_deser(std::string_view const in)
{
  if (in == "executable")
    return Enum::Executable;
  return std::nullopt;
}

std::string_view
enum_ser(Enum const in)
{
  switch (in) {
    case Enum::Executable:
      return "executable";
  }

  std::unreachable();
}

using enum_descriptor = scl::enum_field_descriptor<Enum, enum_deser, enum_ser>;

struct sub
{
  using scl_fields = std::tuple<>;
  using scl_recurse = std::tuple<>;
};

struct e
{
  int b;
  using scl_fields = std::tuple<scl::field<&e::b, "b">>;
};

struct test
{
  std::vector<sub> m;
  e s;

  // using scl_fields = std::tuple<scl::field<&test::m, "halo">>;
  using scl_recurse =
    std::tuple<scl::field<&test::m, "m">, scl::field<&test::s, "s">>;
};

std::string
open_test_file()
{
  std::ifstream f("./test/test_file.txt");

  if (f.fail())
    throw std::runtime_error("unable to get test file");

  std::stringstream ss;
  ss << f.rdbuf();

  return ss.str();
}

int
main()
{
  scl::file scl(open_test_file());
  test t;
  // scl::deserialize(t, scl, "test");

  scl::deserialize(t, scl);

  // for (auto const& m : t.x)
  //   std::cout << m << std::endl;

  scl::file e;
  scl::serialize(t, e);
  std::cout << e.serialize();
}
