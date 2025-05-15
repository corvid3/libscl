#include "../src/scl.hh"
#include <format>
#include <fstream>
#include <iostream>
#include <regex>
#include <tuple>
#include <vector>

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
  scl::scl_file scl(open_test_file());
  test t;
  // scl::deserialize(t, scl, "test");

  scl::deserialize(t, scl, "test");

  for (auto const& m : t.x)
    std::cout << m << std::endl;

  scl.serialize();
}
