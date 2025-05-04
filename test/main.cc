#include "../src/scl.hh"
#include <format>
#include <iostream>
#include <tuple>

auto const file_contents = R"(
# test

[test]
bag = "bilbo"

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
  scl::string b;

  inner_t inner;

  using scl_fields = scl::field_descriptor<scl::field<&test::b, "bag"_f>>;
  using scl_recurse =
    scl::field_descriptor<scl::field<&test::inner, "inner"_f>>;
};

int
main()
{
  scl::scl_file scl(file_contents);

  test t;
  scl::deserialize(t, scl, "test");

  scl::scl_file n;
  scl::serialize(t, n, "test");

  test t2;
  scl::deserialize(t2, n, "test");

  std::cout << std::format("{}, {}", t2.b, t2.inner.agga);
}
