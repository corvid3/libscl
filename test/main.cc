#include "../src/scl.hh"
#include <format>
#include <iostream>
#include <tuple>

auto const file_contents = R"(
# test
[structure]
str = "hello, world!"
)";

using scl::operator""_f;

struct test
{
  scl::number a;
  scl::string b;

  using scl_fields = scl::field_descriptor<scl::field<&test::a, "foo"_f, 4>,
                                           scl::field<&test::b, "str"_f>>;
};

int
main()
{
  scl::scl_file scl(file_contents);

  test m;
  scl::deserialize(m, scl, "structure");

  scl::scl_file into;
  scl::serialize(m, into, "structure");

  test m2;
  scl::deserialize(m2, into, "structure");

  std::cout << std::format("{} {}\n", m2.a, m2.b);
}
