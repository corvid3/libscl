#include "../src/scl.hh"
#include <format>
#include <iostream>
#include <tuple>

auto const file_contents = R"(
# test

[test]
bag = { 1 2 3 }

[test.inner]
agga = "inbob"

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
  std::tuple<scl::number, scl::number, scl::number> c;

  inner_t inner;

  using scl_fields = scl::field_descriptor<scl::field<&test::c, "bag"_f>>;
  using scl_recurse =
    scl::field_descriptor<scl::field<&test::inner, "inner"_f>>;
};

int
main()
{
  scl::scl_file scl(file_contents);

  test t;
  scl::deserialize(t, scl, "test");

  std::cout << std::format(
    "{} {} {}", std::get<0>(t.c), std::get<1>(t.c), std::get<2>(t.c));
}
