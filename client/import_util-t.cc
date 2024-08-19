/* Copyright (c) 2024, MariaDB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1335  USA
 */

#include "my_config.h"
#include "import_util.h"

#include <tap.h>
/*
  Test parsing of CREATE TABLE in mariadb-import utility
*/
static void test_ddl_parser()
{
  std::string script= R"(
     -- Some SQL script
 CREATE TABLE `book` (
  `id` mediumint(8) unsigned NOT NULL AUTO_INCREMENT,
  `title` varchar(200) NOT NULL,
  `author_id` smallint(5) unsigned NOT NULL,
  PRIMARY KEY (`id`),
  KEY `fk_book_author` (`author_id`),
  CONSTRAINT `fk_book_author` FOREIGN KEY (`author_id`) REFERENCES `author` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_uca1400_ai_ci;
)";

  auto create_table_stmt= extract_first_create_table(script);
  ok(!create_table_stmt.empty(), "CREATE TABLE statement found");

  TableDDLInfo table_definitions(create_table_stmt);
  const std::string& pkdef= table_definitions.primary_key.definition;
  const std::string& table_name= table_definitions.table_name;
  const std::string& storage_engine= table_definitions.storage_engine;

  ok(table_name == "`book`", "Table name is %s",table_name.c_str());
  ok(storage_engine == "InnoDB", "Storage engine is %s", storage_engine.c_str());
  ok(pkdef == "PRIMARY KEY (`id`)", "Primary key def is %s",pkdef.c_str());

  ok(table_definitions.secondary_indexes.size() == 1, "Secondary indexes size correct");
  const auto &sec_index= table_definitions.secondary_indexes[0];
  ok(sec_index.definition == "KEY `fk_book_author` (`author_id`)",
      "Secondary index definition is %s",sec_index.definition.c_str());
  ok(sec_index.name == "`fk_book_author`", "Secondary index name is %s",
     sec_index.name.c_str());

  ok(table_definitions.constraints.size() == 1, "Constraints size correct");
  const auto &constraint= table_definitions.constraints[0];
  ok(constraint.definition ==
     "CONSTRAINT `fk_book_author` FOREIGN KEY (`author_id`) REFERENCES "
     "`author` (`id`) ON DELETE CASCADE",
     "Constraint definition %s", constraint.definition.c_str());

  ok(constraint.name == "`fk_book_author`",
     "Constraint name is %s", constraint.name.c_str());
  std::string drop_constraints= table_definitions.drop_constraints_sql();
  ok(drop_constraints ==
     "ALTER TABLE `book` DROP CONSTRAINT `fk_book_author`",
     "Drop constraints SQL is \"%s\"", drop_constraints.c_str());
  std::string add_constraints= table_definitions.add_constraints_sql();
  ok(add_constraints ==
      "ALTER TABLE `book` ADD CONSTRAINT `fk_book_author` FOREIGN KEY "
     "(`author_id`) REFERENCES `author` (`id`) ON DELETE CASCADE",
      "Add constraints SQL is \"%s\"",add_constraints.c_str());

  std::string drop_secondary_indexes=
      table_definitions.drop_secondary_indexes_sql();
  ok(drop_secondary_indexes == "ALTER TABLE `book` DROP INDEX `fk_book_author`",
     "Drop secondary indexes SQL is \"%s\"", drop_secondary_indexes.c_str());
  std::string add_secondary_indexes=
      table_definitions.add_secondary_indexes_sql();
  ok(add_secondary_indexes == "ALTER TABLE `book` ADD KEY `fk_book_author` (`author_id`)",
     "Add secondary indexes SQL is \"%s\"", add_secondary_indexes.c_str());
}

/*
 For Innodb table without PK, and but with Unique key
 (which is used for clustering, instead of PK)
 this key will not be added and dropped by
 the import utility
*/
static void innodb_non_pk_clustering_key()
{
  auto create_table_stmt= R"(
  CREATE TABLE `book` (
  `id` mediumint(8),
   UNIQUE KEY `id` (`id`)
  ) ENGINE=InnoDB;
 )";
  TableDDLInfo table_definitions(create_table_stmt);
  ok(table_definitions.non_pk_clustering_key_name == "`id`",
     "Non-PK clustering key is %s",
     table_definitions.non_pk_clustering_key_name.c_str());
  ok(table_definitions.primary_key.definition.empty(),
     "Primary key is %s", table_definitions.primary_key.definition.c_str());
  ok(table_definitions.secondary_indexes.size() == 1,
     "Secondary indexes size is %zu",
     table_definitions.secondary_indexes.size());
  ok(table_definitions.add_secondary_indexes_sql().empty(),
     "No secondary indexes to add");
  ok(table_definitions.drop_secondary_indexes_sql().empty(),
     "No secondary indexes to drop");
}
int main()
{
  plan(19);
  diag("Testing DDL parser");

  test_ddl_parser();
  innodb_non_pk_clustering_key();
  return exit_status();
}
