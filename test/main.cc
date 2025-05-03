#include "../src/scl.hh"
#include <format>
#include <iostream>
#include <tuple>

auto const file_contents = R"(
# test
[agg]
[[structure]]
str = "hello, world!"
[[structure]]
str = "fooington"
[[structure]]
str = "barleybaz"
)";

using scl::operator""_f;

struct test
{
  scl::string b;

  using scl_fields = scl::field_descriptor<scl::field<&test::b, "str"_f>>;
};

int
main()
{
  scl::scl_file scl(file_contents);

  std::vector<test> m;
  scl::deserialize(std::inserter(m, m.end()), scl, "structure");

  scl::scl_file into;
  scl::serialize<test>(m, into, "structure");

  std::vector<test> m2;
  scl::deserialize(std::inserter(m2, m2.end()), scl, "structure");

  for (auto const& m : m2)
    std::cout << std::format("{}\n", m.b);
}
