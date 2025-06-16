#include "../src/scl.hh"
#include <format>
#include <fstream>
#include <iostream>
#include <optional>
#include <regex>
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

struct test
{
  std::vector<std::string> x;
  Enum e;
  std::optional<std::string> m;

  using scl_fields = std::tuple<
    // scl::field<&test::x, "foogle">,
    scl::field<&test::m, "halo">
    // scl::enum_field<&test::e, "enum", enum_descriptor>
    >;

  // inner_t inner;
  // using scl_recurse =
  //   scl::field_descriptor<scl::field<&test::inner, "inner"_f>>;
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

  scl::deserialize(t, scl, "test");

  // for (auto const& m : t.x)
  //   std::cout << m << std::endl;

  scl::file e;
  scl::serialize(t, e, "test");
  std::cout << e.serialize();
}
