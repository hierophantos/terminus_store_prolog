void* open_directory_store(char* dir_name);
void cleanup_directory_store(void* store);
void cleanup_db(void* db);
void cleanup_layer(void* layer);
void cleanup_layer_builder(void* layer_builder);
void cleanup_cstring(char* c_string);
void* create_database(void* store, char* name, char** err);
void* open_database(void* store, char* name, char** err);
void* database_get_head(void* db, char** err);
void* store_create_base_layer(void* db, char** err);
void* layer_open_write(void* layer, char** err);
int builder_add_id_triple(void* builder, uint64_t subject, uint64_t predicate, uint64_t object, char** err);
void builder_add_string_node_triple(void* builder, char* subject, char* predicate, char* object, char** err);
void builder_add_string_value_triple(void* builder, char* subject, char* predicate, char* object, char** err);
int builder_remove_id_triple(void* builder, uint64_t subject, uint64_t predicate, uint64_t object, char** err);
int builder_remove_string_node_triple(void* builder, char* subject, char* predicate, char* object, char** err);
int builder_remove_string_value_triple(void* builder, char* subject, char* predicate, char* object, char** err);
void* builder_commit(void* builder, char** err);
