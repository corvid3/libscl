#include "../src/scl.hh"
#include <format>
#include <iostream>
#include <regex>
#include <tuple>

auto const file_contents = R"(
# test

[test]
foogle = "ag"
spring = "adrarstr"
toog = t
foogble = "ag"
str = "str"

)";

using scl::operator""_f;

struct inner_t
{
  scl::string agga;

  using scl_fields =
    scl::field_descriptor<scl::field<&inner_t::agga, "agga"_f>>;
};

struct test
{
  std::string x;

  inner_t inner;

  using scl_fields = scl::field_descriptor<scl::field<&test::x, "foogle"_f>>;
  // using scl_recurse =
  //   scl::field_descriptor<scl::field<&test::inner, "inner"_f>>;
};

int
main()
{
  scl::scl_file scl(file_contents);
  test t;
  scl::deserialize(t, scl, "test");

  std::cout << std::format("{}\n", t.x);
}
