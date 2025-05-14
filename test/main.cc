#include "../src/scl.hh"
#include <format>
#include <iostream>
#include <regex>
#include <tuple>
#include <vector>

auto const file_contents = R"(
# test

[test]
foogle = { "a" "b" "c" }

[inner]
agga = "string"
noob = false

)";

using scl::operator""_f;

struct inner_t
{};

struct test
{
  std::vector<std::string> x;
  using scl_fields = scl::field_descriptor<scl::field<&test::x, "foogle"_f>>;

  // inner_t inner;
  // using scl_recurse =
  //   scl::field_descriptor<scl::field<&test::inner, "inner"_f>>;
};

int
main()
{
  scl::scl_file scl(file_contents);
  test t;
  // scl::deserialize(t, scl, "test");

  scl::deserialize(t, scl, "test");

  for (auto const& m : t.x)
    std::cout << m << std::endl;

  scl.serialize();
}
