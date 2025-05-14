#include "../src/scl.hh"
#include <format>
#include <iostream>
#include <regex>
#include <tuple>

auto const file_contents = R"(
# test

[test]
foogle = {1 2 3 4}

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
  std::vector<int> x;
  // int x;

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

  for (auto const& i : t.x)
    std::cout << std::format("{}\n", i);

  scl::serialize(t, scl, "test");
}
