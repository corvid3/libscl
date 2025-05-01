#include "../src/scl.hh"
#include <format>
#include <iostream>

auto const file_contents = R"(
[table_name]
[table.dot]
m = 22.2
)";

int
main()
{
  scl::scl_file scl(file_contents);

  std::cout << std::format("num tables: {}\n", scl.num_tables());

  auto const table = scl.get_table("table.dot").second;
  std::cout << std::format("{}\n", table.at("m").as_num());

  // std::cout << std::format("{}, {}\n", n, k.as_num());
}
