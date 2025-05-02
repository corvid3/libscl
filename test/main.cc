#include "../src/scl.hh"
#include <format>
#include <iostream>

auto const file_contents = R"(
# test
[table_name]
# test
[table.dot]
# test
m = {1 2 3}
# test
)";

int
main()
{
  scl::scl_file scl(file_contents);

  std::cout << std::format("num tables: {}\n", scl.num_tables());

  auto m = scl.get_table("table_name");

  auto const table = scl.get_table("table.dot").second;
  auto const arr =
    table.at("m").get<scl::array>("expected an array at m in table");
  for (auto const& v : arr) {
    std::cout << std::format("{}\n",
                             v.get<scl::number>("m should be a number"));
  }
}
