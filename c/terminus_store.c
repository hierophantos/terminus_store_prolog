#include <assert.h>
#include <SWI-Prolog.h>
#include <SWI-Stream.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include "terminus_store.h"
#include "error.h"
#include "blobs.h"

static foreign_t pl_csv_iri(term_t csv_name_term, term_t prefix_term, term_t iri_term){
    char* csv_name = check_string_or_atom_term(csv_name_term);
    char* prefix = check_string_or_atom_term(prefix_term);
    char* str_ptr = csv_iri(csv_name, prefix);
    foreign_t result = PL_unify_string_chars(iri_term, str_ptr);
    cleanup_cstring(str_ptr);
    return result;
}

static foreign_t pl_apply_diff(term_t builder_blob,
                               term_t layer_blob) {
    void* builder = check_blob_type(builder_blob, &layer_builder_blob_type);
    void* layer = check_blob_type(layer_blob, &layer_blob_type);

    char* err;
    builder_apply_diff(builder,layer,&err);

    if (err != NULL) {
        return throw_rust_err(err);
    }
    PL_succeed;
}

static foreign_t pl_csv_builder_with_schema(term_t name_term,
                                            term_t csv_term,
                                            term_t builder_blob,
                                            term_t schema_builder_blob,
                                            term_t data_prefix_term,
                                            term_t schema_prefix_term,
                                            term_t header_term,
                                            term_t skip_header_term) {

    char* name = check_string_or_atom_term(name_term);
    char* csv = check_string_or_atom_term(csv_term);
    void* builder = check_blob_type(builder_blob, &layer_builder_blob_type);
    void* schema_builder = check_blob_type(schema_builder_blob, &layer_builder_blob_type);
    char* data_prefix = check_string_or_atom_term(data_prefix_term);
    char* schema_prefix = check_string_or_atom_term(schema_prefix_term);

    int header;
    PL_get_bool_ex(header_term,&header);

    int skip_header;
    PL_get_bool_ex(skip_header_term,&skip_header);

    char* err;
    add_csv_to_builder(name,
                       csv,
                       builder,
                       schema_builder,
                       data_prefix,
                       schema_prefix,
                       header,
                       skip_header,
                       &err);

    if (err != NULL) {
        return throw_rust_err(err);
    }
    PL_succeed;
}

static foreign_t pl_csv_builder(term_t name_term,
                                term_t csv_term,
                                term_t builder_blob,
                                term_t data_prefix_term,
                                term_t schema_prefix_term,
                                term_t header_term,
                                term_t skip_header_term) {

    char* name = check_string_or_atom_term(name_term);
    char* csv = check_string_or_atom_term(csv_term);
    void* builder = check_blob_type(builder_blob, &layer_builder_blob_type);
    char* data_prefix = check_string_or_atom_term(data_prefix_term);
    char* schema_prefix = check_string_or_atom_term(schema_prefix_term);

    int header;
    PL_get_bool_ex(header_term,&header);

    int skip_header;
    PL_get_bool_ex(skip_header_term,&skip_header);

    char* err;
    add_csv_to_builder(name,
                       csv,
                       builder,
                       NULL,
                       data_prefix,
                       schema_prefix,
                       header,
                       skip_header,
                       &err);

    if (err != NULL) {
        return throw_rust_err(err);
    }
    PL_succeed;
}

static foreign_t pl_open_memory_store(term_t store_term) {
    if (!PL_is_variable(store_term)) {
        PL_fail;
    }
    void* store_ptr = open_memory_store();
    PL_unify_blob(store_term, store_ptr, 0, &store_blob_type);
    PL_succeed;
}

static foreign_t pl_open_directory_store(term_t dir_name_term, term_t store_term) {
    if (!PL_is_variable(store_term)) {
        PL_fail;
    }

    char* dir_name = check_string_or_atom_term(dir_name_term);
    void* store_ptr = open_directory_store(dir_name);
    PL_unify_blob(store_term, store_ptr, 0, &store_blob_type);

    PL_succeed;
}

static foreign_t pl_create_named_graph(term_t store_blob, term_t db_name_term, term_t db_term) {
    void* store = check_blob_type(store_blob, &store_blob_type);
    char* db_name = check_string_or_atom_term(db_name_term);

    if (!PL_is_variable(db_term)) {
        PL_fail;
    }

    char* err;
    void* db_ptr = create_named_graph(store, db_name, &err);
    // Decent error handling, not only checking for null
    if (db_ptr == NULL) {
        return throw_rust_err(err);
    }
    PL_unify_blob(db_term, db_ptr, 0, &named_graph_blob_type);
    PL_succeed;
}

static foreign_t pl_open_named_graph(term_t store_blob, term_t db_name_term, term_t db_term) {
    void* store = check_blob_type(store_blob, &store_blob_type);
    char* db_name = check_string_or_atom_term(db_name_term);

    if (!PL_is_variable(db_term)) {
        PL_fail;
    }

    char* err;
    void* db_ptr = open_named_graph(store, db_name, &err);
    if (db_ptr == NULL) {
        if (err != NULL) {
            return throw_rust_err(err);
        }
        PL_fail;
    }
    else {
        PL_unify_blob(db_term, db_ptr, 0, &named_graph_blob_type);
        PL_succeed;
    }
}

static foreign_t pl_head(term_t named_graph_blob_term, term_t layer_term) {
    void* named_graph = check_blob_type(named_graph_blob_term, &named_graph_blob_type);
    if (!PL_is_variable(layer_term)) {
        PL_fail;
    }

    char* err;
    void* layer_ptr = named_graph_get_head(named_graph, &err);
    if (layer_ptr == NULL) {
        if (err == NULL) {
            PL_fail;
        }
        else {
            return throw_rust_err(err);
        }
    }
    else {
        PL_unify_blob(layer_term, layer_ptr, 0, &layer_blob_type);
    }

    PL_succeed;
}

static foreign_t pl_set_head(term_t named_graph_blob_term, term_t layer_blob_term) {
    void* named_graph = check_blob_type(named_graph_blob_term, &named_graph_blob_type);
    void* layer = check_blob_type(layer_blob_term, &layer_blob_type);

    char* err;
    if (named_graph_set_head(named_graph, layer, &err)) {
        PL_succeed;
    }
    else {
        PL_fail;
    }
}

static foreign_t pl_force_set_head(term_t named_graph_blob_term, term_t layer_blob_term) {
    void* named_graph = check_blob_type(named_graph_blob_term, &named_graph_blob_type);
    void* layer = check_blob_type(layer_blob_term, &layer_blob_type);

    char* err;
    if (named_graph_force_set_head(named_graph, layer, &err)) {
        PL_succeed;
    }
    else {
        PL_fail;
    }
}

static foreign_t pl_open_write(term_t layer_or_named_graph_or_store_term, term_t builder_term) {
    if (PL_is_variable(layer_or_named_graph_or_store_term)) {
        return throw_instantiation_err(layer_or_named_graph_or_store_term);
    }

    PL_blob_t* blob_type;
    void* blob;
    if (!PL_get_blob(layer_or_named_graph_or_store_term, &blob, NULL, &blob_type) || (blob_type != &store_blob_type && blob_type != &layer_blob_type && blob_type != &named_graph_blob_type)) {
        return throw_type_error(layer_or_named_graph_or_store_term, "layer");
    }

    if (!PL_is_variable(builder_term)) {
        PL_fail;
    }

    char* err;
    void* builder_ptr;
    if (blob_type == &store_blob_type) {
        builder_ptr = store_create_base_layer(blob, &err);
    }
    else if (blob_type == &layer_blob_type) {
        builder_ptr = layer_open_write(blob, &err);
    }
    else if (blob_type == &named_graph_blob_type) {
        builder_ptr = named_graph_open_write(blob, &err);
    }
    else {
        abort();
    }

    if (builder_ptr == NULL) {
        return throw_rust_err(err);
    }
    else {
        PL_unify_blob(builder_term, builder_ptr, 0, &layer_builder_blob_type);
    }

    PL_succeed;
}

static foreign_t pl_add_id_triple(term_t builder_term, term_t subject_term, term_t predicate_term, term_t object_term) {
    void* builder = check_blob_type(builder_term, &layer_builder_blob_type);
    uint64_t subject, predicate, object;
    if (!PL_cvt_i_uint64(subject_term, &subject)) {
        PL_fail;
    }
    if (!PL_cvt_i_uint64(predicate_term, &predicate)) {
        PL_fail;
    }
    if (!PL_cvt_i_uint64(object_term, &object)) {
        PL_fail;
    }

    char *err;
    builder_add_id_triple(builder, subject, predicate, object, &err);
    if (err != NULL) {
        return throw_rust_err(err);
    }

    PL_succeed;
}

static foreign_t pl_add_string_node_triple(term_t builder_term, term_t subject_term, term_t predicate_term, term_t object_term) {
    void* builder = check_blob_type(builder_term, &layer_builder_blob_type);
    char* subject = check_string_or_atom_term(subject_term);
    char* predicate = check_string_or_atom_term(predicate_term);
    char* object = check_string_or_atom_term(object_term);

    char *err;
    builder_add_string_node_triple(builder, subject, predicate, object, &err);
    if (err != NULL) {
        return throw_rust_err(err);
    }

    PL_succeed;
}

static foreign_t pl_add_string_value_triple(term_t builder_term, term_t subject_term, term_t predicate_term, term_t object_term) {
    void* builder = check_blob_type(builder_term, &layer_builder_blob_type);
    char* subject = check_string_or_atom_term(subject_term);
    char* predicate = check_string_or_atom_term(predicate_term);
    char* object = check_string_or_atom_term(object_term);

    char *err;
    builder_add_string_value_triple(builder, subject, predicate, object, &err);
    if (err != NULL) {
        return throw_rust_err(err);
    }

    PL_succeed;
}

static foreign_t pl_remove_id_triple(term_t builder_term, term_t subject_term, term_t predicate_term, term_t object_term) {
    void* builder = check_blob_type(builder_term, &layer_builder_blob_type);
    uint64_t subject, predicate, object;
    if (!PL_cvt_i_uint64(subject_term, &subject)) {
        PL_fail;
    }
    if (!PL_cvt_i_uint64(predicate_term, &predicate)) {
        PL_fail;
    }
    if (!PL_cvt_i_uint64(object_term, &object)) {
        PL_fail;
    }

    char *err;
    builder_remove_id_triple(builder, subject, predicate, object, &err);
    if (err != NULL) {
        return throw_rust_err(err);
    }

    PL_succeed;
}

static foreign_t pl_remove_string_node_triple(term_t builder_term, term_t subject_term, term_t predicate_term, term_t object_term) {
    void* builder = check_blob_type(builder_term, &layer_builder_blob_type);
    char* subject = check_string_or_atom_term(subject_term);
    char* predicate = check_string_or_atom_term(predicate_term);
    char* object = check_string_or_atom_term(object_term);

    char *err;
    builder_remove_string_node_triple(builder, subject, predicate, object, &err);
    if (err != NULL) {
        return throw_rust_err(err);
    }

    PL_succeed;
}

static foreign_t pl_remove_string_value_triple(term_t builder_term, term_t subject_term, term_t predicate_term, term_t object_term) {
    void* builder = check_blob_type(builder_term, &layer_builder_blob_type);
    char* subject = check_string_or_atom_term(subject_term);
    char* predicate = check_string_or_atom_term(predicate_term);
    char* object = check_string_or_atom_term(object_term);
    char *err;
    builder_remove_string_value_triple(builder, subject, predicate, object, &err);

    if (err != NULL) {
        return throw_rust_err(err);
    }

    PL_succeed;
}

static foreign_t pl_builder_committed(term_t builder_term) {
    void* builder = check_blob_type(builder_term, &layer_builder_blob_type);

    if (builder_committed(builder)) {
        PL_succeed;
    }
    else {
        PL_fail;
    }
}

static foreign_t pl_builder_commit(term_t builder_term, term_t layer_term) {
    if (!PL_is_variable(layer_term)) {
        PL_fail;
    }

    void* builder = check_blob_type(builder_term, &layer_builder_blob_type);

    char* err;
    void* layer_ptr = builder_commit(builder, &err);
    if (layer_ptr == NULL) {
        return throw_rust_err(err);
    }

    PL_unify_blob(layer_term, layer_ptr, 0, &layer_blob_type);
    PL_succeed;
}

static foreign_t pl_apply_delta(term_t builder_term, term_t layer_term) {
    void* builder = check_blob_type(builder_term, &layer_builder_blob_type);
    void* layer = check_blob_type(layer_term, &layer_blob_type);

    char* err;
    builder_apply_delta(builder, layer, &err);
    if (err != NULL) {
        return throw_rust_err(err);
    }

    PL_succeed;
}

static foreign_t pl_node_and_value_count(term_t layer_term, term_t count_term) {
    void* layer = check_blob_type(layer_term, &layer_blob_type);

    uint64_t count = (uint64_t) layer_node_and_value_count(layer);

    return PL_unify_uint64(count_term, count);
}

static foreign_t pl_predicate_count(term_t layer_term, term_t count_term) {
    void* layer = check_blob_type(layer_term, &layer_blob_type);

    uint64_t count = (uint64_t) layer_predicate_count(layer);

    return PL_unify_uint64(count_term, count);
}

static foreign_t pl_subject_to_id(term_t layer_term, term_t subject_term, term_t id_term) {
    void* layer = check_blob_type(layer_term, &layer_blob_type);
    if (PL_is_variable(subject_term)) {
        return throw_instantiation_err(subject_term);
    }
    else {
        char* subject = check_string_or_atom_term(subject_term);
        uint64_t id = layer_subject_id(layer, subject);

        if (id == 0) {
            PL_fail;
        }

        return PL_unify_uint64(id_term, id);
    }
}

static foreign_t pl_id_to_subject(term_t layer_term, term_t id_term, term_t subject_term) {
    void* layer = check_blob_type(layer_term, &layer_blob_type);
    if (PL_is_variable(id_term)) {
        return throw_instantiation_err(id_term);
    }
    else {
        uint64_t id;
        if (!PL_cvt_i_uint64(id_term, &id)) {
            PL_fail;
        }

        char* subject = layer_id_subject(layer, id);
        if (subject == NULL) {
            PL_fail;
        }
        int result = PL_unify_chars(subject_term, PL_STRING|REP_UTF8, -1, subject);
        cleanup_cstring(subject);

        return result;
    }
}

static foreign_t pl_predicate_to_id(term_t layer_term, term_t predicate_term, term_t id_term) {
    void* layer = check_blob_type(layer_term, &layer_blob_type);
    if (PL_is_variable(predicate_term)) {
        return throw_instantiation_err(predicate_term);
    }
    else {
        char* predicate = check_string_or_atom_term(predicate_term);
        uint64_t id = layer_predicate_id(layer, predicate);
        if (id == 0) {
            PL_fail;
        }

        return PL_unify_uint64(id_term, id);
    }
}

static foreign_t pl_id_to_predicate(term_t layer_term, term_t id_term, term_t predicate_term) {
    void* layer = check_blob_type(layer_term, &layer_blob_type);
    if (PL_is_variable(id_term)) {
        return throw_instantiation_err(id_term);
    }
    else {
        uint64_t id;
        if (!PL_cvt_i_uint64(id_term, &id)) {
            PL_fail;
        }

        char* predicate = layer_id_predicate(layer, id);
        if (predicate == NULL) {
            PL_fail;
        }
        int result = PL_unify_chars(predicate_term, PL_STRING|REP_UTF8, -1, predicate);
        cleanup_cstring(predicate);

        return result;
    }
}

static foreign_t pl_object_node_to_id(term_t layer_term, term_t object_term, term_t id_term) {
    void* layer = check_blob_type(layer_term, &layer_blob_type);
    if (PL_is_variable(object_term)) {
        return throw_instantiation_err(object_term);
    }
    else {
        char* object = check_string_or_atom_term(object_term);
        uint64_t id = layer_object_node_id(layer, object);

        if (id == 0) {
            PL_fail;
        }

        return PL_unify_uint64(id_term, id);
    }
}

static foreign_t pl_object_value_to_id(term_t layer_term, term_t object_term, term_t id_term) {
    void* layer = check_blob_type(layer_term, &layer_blob_type);
    if (PL_is_variable(object_term)) {
        return throw_instantiation_err(object_term);
    }
    else {
        char* object = check_string_or_atom_term(object_term);
        uint64_t id = layer_object_value_id(layer, object);

        if (id == 0) {
            PL_fail;
        }

        return PL_unify_uint64(id_term, id);
    }
}

static foreign_t pl_id_to_object(term_t layer_term, term_t id_term, term_t object_term, term_t object_type_term) {
    void* layer = check_blob_type(layer_term, &layer_blob_type);
    if (PL_is_variable(id_term)) {
        return throw_instantiation_err(id_term);
    }
    else {
        uint64_t id;
        if (!PL_cvt_i_uint64(id_term, &id)) {
            PL_fail;
        }

        uint8_t object_type;
        char* object = layer_id_object(layer, id, &object_type);
        if (object == NULL) {
            PL_fail;
        }
        if (object_type == 0) {
            if (!PL_unify_atom_chars(object_type_term, "node")) {
                PL_fail;
            }
        }
        else if (object_type == 1) {
            if (!PL_unify_atom_chars(object_type_term, "value")) {
                PL_fail;
            }
        }
        else {
            abort();
        }
        int result = PL_unify_chars(object_term, PL_STRING|REP_UTF8, -1, object);
        cleanup_cstring(object);

        return result;
    }
}

static foreign_t pl_layer_parent(term_t layer_term, term_t parent_term) {
    void* layer = check_blob_type(layer_term, &layer_blob_type);

    if (PL_is_variable(layer_term)) {
        return throw_instantiation_err(layer_term);
    }

    if (!PL_is_variable(parent_term)) {
        PL_fail;
    }

    char* err;
    void* parent = layer_parent(layer, &err);
    if (parent == NULL) {
        if (err) {
            return throw_rust_err(err);
        }
        else {
            PL_fail;
        }
    }

    PL_unify_blob(parent_term, parent, 0, &layer_blob_type);

    PL_succeed;
}

static foreign_t pl_layer_squash(term_t layer_term, term_t squash_layer_term) {
    void* layer = check_blob_type(layer_term, &layer_blob_type);

    if (PL_is_variable(layer_term)) {
        return throw_instantiation_err(layer_term);
    }

    if (!PL_is_variable(squash_layer_term)) {
        PL_fail;
    }

    char* err;
    void* squash_ptr = layer_squash(layer, &err);
    if (squash_ptr == NULL) {
        return throw_rust_err(err);
    }

    PL_unify_blob(squash_layer_term, squash_ptr, 0, &layer_blob_type);

    PL_succeed;
}

static foreign_t pl_layer_rollup(term_t layer_term) {
    void* layer = check_blob_type(layer_term, &layer_blob_type);

    if (PL_is_variable(layer_term)) {
        return throw_instantiation_err(layer_term);
    }

    char* err;
    layer_rollup(layer, &err);
    if (err != NULL) {
        return throw_rust_err(err);
    }
    PL_succeed;
}

static foreign_t pl_layer_addition_count(term_t layer_term, term_t count_term) {
    void* layer = check_blob_type(layer_term, &layer_blob_type);

    char* err = NULL;
    uint64_t count = (uint64_t) layer_triple_addition_count(layer, &err);

    if (err != NULL) {
        return throw_rust_err(err);
    }

    return PL_unify_uint64(count_term, count);
}

static foreign_t pl_layer_removal_count(term_t layer_term, term_t count_term) {
    void* layer = check_blob_type(layer_term, &layer_blob_type);

    char* err = NULL;
    uint64_t count = (uint64_t) layer_triple_removal_count(layer, &err);

    if (err != NULL) {
        return throw_rust_err(err);
    }

    return PL_unify_uint64(count_term, count);
}

static foreign_t pl_layer_total_addition_count(term_t layer_term, term_t count_term) {
    void* layer = check_blob_type(layer_term, &layer_blob_type);

    uint64_t count = (uint64_t) layer_total_triple_addition_count(layer);

    return PL_unify_uint64(count_term, count);
}

static foreign_t pl_layer_total_removal_count(term_t layer_term, term_t count_term) {
    void* layer = check_blob_type(layer_term, &layer_blob_type);

    uint64_t count = (uint64_t) layer_total_triple_removal_count(layer);

    return PL_unify_uint64(count_term, count);
}

static foreign_t pl_layer_total_triple_count(term_t layer_term, term_t count_term) {
    void* layer = check_blob_type(layer_term, &layer_blob_type);

    uint64_t count = (uint64_t) layer_total_triple_count(layer);

    return PL_unify_uint64(count_term, count);
}

static foreign_t pl_num_store_blobs(term_t num) {
    return PL_unify_uint64(num, n_store_blobs);
}

static foreign_t pl_num_named_graph_blobs(term_t num) {
    return PL_unify_uint64(num, n_named_graph_blobs);
}

static foreign_t pl_num_layer_blobs(term_t num) {
    return PL_unify_uint64(num, n_layer_blobs);
}

static foreign_t pl_num_layer_builder_blobs(term_t num) {
    return PL_unify_uint64(num, n_layer_builder_blobs);
}

static foreign_t pl_layer_to_id(term_t layer_term, term_t id_term) {
    void* layer = check_blob_type(layer_term, &layer_blob_type);

    char* id = layer_get_id(layer);
    foreign_t result = PL_unify_chars(id_term, PL_STRING|REP_UTF8, -1, id);
    cleanup_cstring(id);

    return result;
}

static foreign_t pl_store_id_layer(term_t store_term, term_t id_term, term_t layer_term) {
    void* store = check_blob_type(store_term, &store_blob_type);

    if (PL_is_variable(id_term)) {
        if (PL_is_variable(layer_term)) {
            return throw_instantiation_err(layer_term);
        }

        return pl_layer_to_id(layer_term, id_term);
    }

    char* id = check_string_or_atom_term(id_term);
    char* err;
    void* layer = store_get_layer_from_id(store, id, &err);
    if (layer == NULL) {
        if (err != NULL) {
            return throw_rust_err(err);
        }

        PL_fail;
    }

    return PL_unify_blob(layer_term, layer, 0, &layer_blob_type);
}

static foreign_t build_layer_id_array(term_t layer_ids_term, uint32_t (**layer_id_array)[5], size_t *len) {
    if (PL_skip_list(layer_ids_term, 0, len) != PL_LIST) {
        return throw_c_err("layer ids are not a proper list");
    }

    *layer_id_array = malloc(*len * sizeof(uint32_t[5]));

    term_t head_term = PL_new_term_ref();
    term_t list_term = PL_copy_term_ref(layer_ids_term);
    for (int i=0;PL_get_list(list_term, head_term, list_term);i++) {
        if (PL_term_type(head_term) == PL_VARIABLE) {
            throw_instantiation_err(head_term);
        }

        int term_type = PL_term_type(head_term);

        if (term_type != PL_STRING) {
            free(*layer_id_array);
            return throw_type_error(head_term, "string");
        }

        char* id_string;
        size_t id_len;
        foreign_t result = PL_get_string_chars(head_term, &id_string, &id_len);
        if (!result) {
            free(*layer_id_array);
            return result;
        }

        if (id_len != 40) {
            free(*layer_id_array);
            return throw_c_err("layer id string is not 40 characters long");
        }

        char* err;
        layer_string_to_id(id_string, &(*layer_id_array)[i], &err);
        if (err != NULL) {
            free(*layer_id_array);
            return throw_rust_err(err);
        }
    }

    return 1;
}

static foreign_t pl_pack_export(term_t store_term, term_t layer_ids_term, term_t pack_term) {
    void* store = check_blob_type(store_term, &store_blob_type);

    if (PL_is_variable(layer_ids_term)) {
        return throw_instantiation_err(layer_ids_term);
    }

    size_t len;
    uint32_t (*layer_ids)[5] = NULL;

    foreign_t result = build_layer_id_array(layer_ids_term, &layer_ids, &len);
    if (!result) {
        return result;
    }

    VecHandle pack = pack_export(store, layer_ids, len);
    free(layer_ids);

    result = PL_unify_string_nchars(pack_term, pack.len, pack.ptr);
    cleanup_u8_vec(pack);

    return result;
}

static foreign_t pl_pack_layerids_and_parents(term_t pack_term, term_t layer_parents_term) {
    size_t pack_len;
    unsigned char* pack = check_binary_string_term(pack_term, &pack_len);

    char* err = NULL;

    VecHandle handle = pack_layerids_and_parents(pack, pack_len, &err);
    if (err != NULL) {
        return throw_rust_err(err);
    }

    LayerAndParent *layer_parent_ids = handle.ptr;

    term_t head = PL_new_term_refs(handle.len);
    term_t tail = PL_new_term_refs(handle.len);
    term_t list = layer_parents_term;
    foreign_t result;

    atom_t none_atom = PL_new_atom("none");
    atom_t some_atom = PL_new_atom("some");
    atom_t pair_atom = PL_new_atom("-");
    functor_t some_functor = PL_new_functor(some_atom, 1);
    functor_t pair_functor = PL_new_functor(pair_atom, 2);

    for (int i=0;i<handle.len;i++) {
        result = PL_unify_list(list, head, tail);

        if (!result) {
            cleanup_layer_and_parent_vec(handle);

            return result;
        }

        // unify head
        result = PL_unify_functor(head, pair_functor);
        if (!result) {
            cleanup_layer_and_parent_vec(handle);

            return result;
        }
        term_t layer_id_term = PL_new_term_ref();
        term_t layer_parent_id_term = PL_new_term_ref();
        assert(PL_unify_arg(1, head, layer_id_term));
        assert(PL_unify_arg(2, head, layer_parent_id_term));

        char* layer_id = layer_id_to_string(layer_parent_ids[i].layer_id);
        result = PL_unify_string_chars(layer_id_term, layer_id);
        cleanup_cstring(layer_id);

        if (!result) {
            cleanup_layer_and_parent_vec(handle);

            return result;
        }

        if (layer_parent_ids[i].has_parent) {
            result = PL_unify_functor(layer_parent_id_term, some_functor);
            if (!result) {
                cleanup_layer_and_parent_vec(handle);

                return result;
            }

            term_t layer_parent_id_string_term = PL_new_term_ref();
            assert(PL_unify_arg(1, layer_parent_id_term, layer_parent_id_string_term));
            
            char* layer_parent_id = layer_id_to_string(layer_parent_ids[i].layer_parent_id);
            result = PL_unify_string_chars(layer_parent_id_string_term, layer_parent_id);
            cleanup_cstring(layer_parent_id);

            if (!result) {
                cleanup_layer_and_parent_vec(handle);

                return result;
            }
        }
        else {
            result = PL_unify_atom(layer_parent_id_term, none_atom);
            if (!result) {
                cleanup_layer_and_parent_vec(handle);

                return result;
            }
        }

        list = tail;
        head++;
        tail++;
    }

    result = PL_unify_nil(list);

    cleanup_layer_and_parent_vec(handle);

    return result;
}

static foreign_t pl_pack_import(term_t store_term, term_t layer_ids_term, term_t pack_term) {
    void* store = check_blob_type(store_term, &store_blob_type);

    if (PL_is_variable(layer_ids_term)) {
        return throw_instantiation_err(layer_ids_term);
    }

    size_t layer_ids_len;
    uint32_t (*layer_ids)[5] = NULL;

    foreign_t result = build_layer_id_array(layer_ids_term, &layer_ids, &layer_ids_len);
    if (!result) {
        return result;
    }

    size_t pack_len;
    unsigned char* pack = check_binary_string_term(pack_term, &pack_len);

    char *err = NULL;
    pack_import(store, pack, pack_len, layer_ids, layer_ids_len, &err);
    free(layer_ids);

    if (err != NULL) {
        throw_rust_err(err);
    }

    PL_succeed;
}

typedef enum {
    SPO,
    SO,
    SP,
    PO,
    P,
    O
} TripleIterType;

typedef struct {
    TripleIterType type;
    void* iter;
} TripleIterControl;

static TripleIterControl* init_triple_iterator(term_t layer_term, term_t subject_term, term_t predicate_term, term_t object_term) {
    void* layer = check_blob_type(layer_term, &layer_blob_type);

    TripleIterControl* control = malloc(sizeof(TripleIterControl));

    if (PL_is_ground(subject_term)) {
        uint64_t subject;
        assert(PL_cvt_i_uint64(subject_term, &subject));

        if (PL_is_ground(predicate_term)) {
            uint64_t predicate;
            assert(PL_cvt_i_uint64(predicate_term, &predicate));

            if (PL_is_ground(object_term)) {
                // we should not be getting here, cause in that case this function ought to not be called.
                abort();
            }
            else {
                control->type = O;
                control->iter = id_triple_sp_iter(layer, subject, predicate);
            }
        }
        else {
            if (PL_is_ground(object_term)) {
                uint64_t object;
                assert(PL_cvt_i_uint64(object_term, &object));

                control->type = P;
                control->iter = id_triple_so_iter(layer, subject, object);
            }
            else {
                control->type = PO;
                control->iter = id_triple_s_iter(layer, subject);
            }
        }
    }
    else if (PL_is_ground(object_term)) {
        uint64_t object;
        assert(PL_cvt_i_uint64(object_term, &object));

        control->type = SP;
        control->iter = id_triple_o_iter(layer, object);
    }
    else if (PL_is_ground(predicate_term)) {
        uint64_t predicate;
        assert(PL_cvt_i_uint64(predicate_term, &predicate));

        control->type = SO;
        control->iter = id_triple_p_iter(layer, predicate);
    }
    else {
        control->type = SPO;
        control->iter = id_triple_iter(layer);
    }

    return control;
}

static TripleIterControl* init_triple_addition_iterator(term_t layer_term, term_t subject_term, term_t predicate_term, term_t object_term, char** err) {
    void* layer = check_blob_type(layer_term, &layer_blob_type);

    TripleIterControl* control = malloc(sizeof(TripleIterControl));

    if (PL_is_ground(subject_term)) {
        uint64_t subject;
        assert(PL_cvt_i_uint64(subject_term, &subject));

        if (PL_is_ground(predicate_term)) {
            uint64_t predicate;
            assert(PL_cvt_i_uint64(predicate_term, &predicate));

            if (PL_is_ground(object_term)) {
                // we should not be getting here, cause in that case this function ought to not be called.
                abort();
            }
            else {
                control->type = O;
                control->iter = id_triple_addition_sp_iter(layer, subject, predicate, err);
            }
        }
        else {
            if (PL_is_ground(object_term)) {
                uint64_t object;
                assert(PL_cvt_i_uint64(object_term, &object));

                control->type = P;
                control->iter = id_triple_addition_so_iter(layer, subject, object, err);
            }
            else {
                control->type = PO;
                control->iter = id_triple_addition_s_iter(layer, subject, err);
            }
        }
    }
    else if (PL_is_ground(predicate_term)) {
        uint64_t predicate;
        assert(PL_cvt_i_uint64(predicate_term, &predicate));

        control->type = SO;
        control->iter = id_triple_addition_p_iter(layer, predicate, err);
    }
    else if (PL_is_ground(object_term)) {
        uint64_t object;
        assert(PL_cvt_i_uint64(object_term, &object));

        control->type = SP;
        control->iter = id_triple_addition_o_iter(layer, object, err);
    }
    else {
        control->type = SPO;
        control->iter = id_triple_addition_iter(layer, err);
    }

    return control;
}

static TripleIterControl* init_triple_removal_iterator(term_t layer_term, term_t subject_term, term_t predicate_term, term_t object_term, char** err) {
    void* layer = check_blob_type(layer_term, &layer_blob_type);

    TripleIterControl* control = malloc(sizeof(TripleIterControl));

    if (PL_is_ground(subject_term)) {
        uint64_t subject;
        assert(PL_cvt_i_uint64(subject_term, &subject));

        if (PL_is_ground(predicate_term)) {
            uint64_t predicate;
            assert(PL_cvt_i_uint64(predicate_term, &predicate));

            if (PL_is_ground(object_term)) {
                // we should not be getting here, cause in that case this function ought to not be called.
                abort();
            }
            else {
                control->type = O;
                control->iter = id_triple_removal_sp_iter(layer, subject, predicate, err);
            }
        }
        else {
            if (PL_is_ground(object_term)) {
                uint64_t object;
                assert(PL_cvt_i_uint64(object_term, &object));

                control->type = P;
                control->iter = id_triple_removal_so_iter(layer, subject, object, err);
            }
            else {
                control->type = PO;
                control->iter = id_triple_removal_s_iter(layer, subject, err);
            }
        }
    }
    else if (PL_is_ground(predicate_term)) {
        uint64_t predicate;
        assert(PL_cvt_i_uint64(predicate_term, &predicate));

        control->type = SO;
        control->iter = id_triple_removal_p_iter(layer, predicate, err);
    }
    else if (PL_is_ground(object_term)) {
        uint64_t object;
        assert(PL_cvt_i_uint64(object_term, &object));

        control->type = SP;
        control->iter = id_triple_removal_o_iter(layer, object, err);
    }
    else {
        control->type = SPO;
        control->iter = id_triple_removal_iter(layer, err);
    }

    return control;
}

static void cleanup_triple_iterator(TripleIterControl* control) {
    switch(control->type) {
    case SPO:
        cleanup_u64_triple_iter(control->iter);
        break;
    case SP:
    case SO:
    case PO:
        cleanup_u64_pair_iter(control->iter);
        break;
    case P:
    case O:
        cleanup_u64_iter(control->iter);
        break;
    default:
        abort();
    }

    free(control);
}

typedef union {
    U64Triple triple;
    U64Pair pair;
    uint64_t single;
} TripleIterElement;

static foreign_t triple_iterator_next(TripleIterControl* control, term_t subject_term, term_t predicate_term, term_t object_term) {
    // loop here cause we'll be unifying, which may fail due to inputs.
    // when failing, we want to try the next result, until a unification does succeed.
    fid_t f = PL_open_foreign_frame();
    for(;;) {
        TripleIterElement element = {0};
        switch(control->type) {
        case SPO:
            element.triple = u64_triple_iter_next(control->iter);
            break;
        case SP:
        case SO:
        case PO:
            element.pair = u64_pair_iter_next(control->iter);
            break;
        case P:
        case O:
            element.single = u64_iter_next(control->iter);
            break;
        default:
            abort();
        }

        if (element.single == 0) {
            // done iterating, nothing more will follow.
            PL_close_foreign_frame(f);
            cleanup_triple_iterator(control);
            PL_fail;
        }
        else {
            int success;
            switch(control->type) {
            case SPO:
                success = PL_unify_uint64(subject_term, element.triple.first)
                    && PL_unify_uint64(predicate_term, element.triple.second)
                    && PL_unify_uint64(object_term, element.triple.third);
                break;
            case SP:
                success = PL_unify_uint64(subject_term, element.pair.first)
                    && PL_unify_uint64(predicate_term, element.pair.second);
                break;
            case SO:
                success = PL_unify_uint64(subject_term, element.pair.first)
                    && PL_unify_uint64(object_term, element.pair.second);
                break;
            case PO:
                success = PL_unify_uint64(predicate_term, element.pair.first)
                    && PL_unify_uint64(object_term, element.pair.second);
                break;
            case P:
                success = PL_unify_uint64(predicate_term, element.single);
                break;
            case O:
                success = PL_unify_uint64(object_term, element.single);
                break;
            default:
                abort();
            }

            if (success) {
                PL_close_foreign_frame(f);
                PL_retry_address(control);
            }
            else {
                PL_rewind_foreign_frame(f);
                continue;
            }
        }
    }
}

static foreign_t pl_id_triple(term_t layer_term, term_t subject_term, term_t predicate_term, term_t object_term, control_t handle) {
    TripleIterControl* control;
    switch (PL_foreign_control(handle)) {
    case PL_FIRST_CALL:
        if (PL_is_ground(subject_term) && PL_is_ground(predicate_term) && PL_is_ground(object_term)) {
            // this is where we do a simple lookup and no type of iteration
            void* layer = check_blob_type(layer_term, &layer_blob_type);
            uint64_t subject, predicate, object;
            assert(PL_cvt_i_uint64(subject_term, &subject));
            assert(PL_cvt_i_uint64(predicate_term, &predicate));
            assert(PL_cvt_i_uint64(object_term, &object));
            if (id_triple_spo_exists(layer, subject, predicate, object)) {
                PL_succeed;
            }
            else {
                PL_fail;
            }
        }
        else {
            // this is where we initialize the iterator
            control = init_triple_iterator(layer_term, subject_term, predicate_term, object_term);
            if (control->iter == NULL) {
                PL_fail;
            }
        }
        break;
    case PL_REDO:
        control = PL_foreign_context_address(handle);
        break;
    case PL_PRUNED:
        control = PL_foreign_context_address(handle);
        cleanup_triple_iterator(control);
        PL_succeed;
    default:
        abort();
    }

    // if we got to this point, we're either in a first time call or a redo.
    // get the next element of the iterator.
    return triple_iterator_next(control, subject_term, predicate_term, object_term);
}

static foreign_t pl_id_triple_addition(term_t layer_term, term_t subject_term, term_t predicate_term, term_t object_term, control_t handle) {
    TripleIterControl* control;
    switch (PL_foreign_control(handle)) {
    case PL_FIRST_CALL:
        if (PL_is_ground(subject_term) && PL_is_ground(predicate_term) && PL_is_ground(object_term)) {
            // this is where we do a simple lookup and no type of iteration
            void* layer = check_blob_type(layer_term, &layer_blob_type);
            uint64_t subject, predicate, object;
            assert(PL_cvt_i_uint64(subject_term, &subject));
            assert(PL_cvt_i_uint64(predicate_term, &predicate));
            assert(PL_cvt_i_uint64(object_term, &object));
            char *err = NULL;
            if (id_triple_addition_spo_exists(layer, subject, predicate, object, &err)) {
                PL_succeed;
            }
            else {
                if (err != NULL) {
                    return throw_rust_err(err);
                }
                PL_fail;
            }
        }
        else {
            // this is where we initialize the iterator
            char* err = NULL;
            control = init_triple_addition_iterator(layer_term, subject_term, predicate_term, object_term, &err);
            if (err != NULL) {
                return throw_rust_err(err);
            }
            if (control->iter == NULL) {
                PL_fail;
            }
        }
        break;
    case PL_REDO:
        control = PL_foreign_context_address(handle);
        break;
    case PL_PRUNED:
        control = PL_foreign_context_address(handle);
        cleanup_triple_iterator(control);
        PL_succeed;
    default:
        abort();
    }

    // if we got to this point, we're either in a first time call or a redo.
    // get the next element of the iterator.
    return triple_iterator_next(control, subject_term, predicate_term, object_term);
}

static foreign_t pl_id_triple_removal(term_t layer_term, term_t subject_term, term_t predicate_term, term_t object_term, control_t handle) {
    TripleIterControl* control;
    switch (PL_foreign_control(handle)) {
    case PL_FIRST_CALL:
        if (PL_is_ground(subject_term) && PL_is_ground(predicate_term) && PL_is_ground(object_term)) {
            // this is where we do a simple lookup and no type of iteration
            void* layer = check_blob_type(layer_term, &layer_blob_type);
            uint64_t subject, predicate, object;
            assert(PL_cvt_i_uint64(subject_term, &subject));
            assert(PL_cvt_i_uint64(predicate_term, &predicate));
            assert(PL_cvt_i_uint64(object_term, &object));
            char* err = NULL;
            if (id_triple_removal_spo_exists(layer, subject, predicate, object, &err)) {
                PL_succeed;
            }
            else {
                if (err != NULL) {
                    return throw_rust_err(err);
                }
                PL_fail;
            }
        }
        else {
            // this is where we initialize the iterator
            char *err = NULL;
            control = init_triple_removal_iterator(layer_term, subject_term, predicate_term, object_term, &err);
            if (err != NULL) {
                return throw_rust_err(err);
            }
            if (control->iter == NULL) {
                PL_fail;
            }
        }
        break;
    case PL_REDO:
        control = PL_foreign_context_address(handle);
        break;
    case PL_PRUNED:
        control = PL_foreign_context_address(handle);
        cleanup_triple_iterator(control);
        PL_succeed;
    default:
        abort();
    }

    // if we got to this point, we're either in a first time call or a redo.
    // get the next element of the iterator.
    return triple_iterator_next(control, subject_term, predicate_term, object_term);
}

                /*****************************************
                 *     Prolog install function           *
                 ****************************************/
install_t
install()
{
    PL_register_foreign("csv_iri", 3,
                        pl_csv_iri, 0);
    PL_register_foreign("csv_builder", 7,
                        pl_csv_builder, 0);
    PL_register_foreign("csv_builder", 8,
                        pl_csv_builder_with_schema, 0);
    PL_register_foreign("open_memory_store", 1,
                        pl_open_memory_store, 0);
    PL_register_foreign("open_directory_store", 2,
                        pl_open_directory_store, 0);
    PL_register_foreign("create_named_graph", 3,
                        pl_create_named_graph, 0);
    PL_register_foreign("open_named_graph", 3,
                        pl_open_named_graph, 0);
    PL_register_foreign("head", 2,
                        pl_head, 0);
    PL_register_foreign("nb_set_head", 2,
                        pl_set_head, 0);
    PL_register_foreign("nb_force_set_head", 2,
                        pl_force_set_head, 0);
    PL_register_foreign("open_write", 2,
                        pl_open_write, 0);
    PL_register_foreign("nb_add_id_triple", 4,
                        pl_add_id_triple, 0);
    PL_register_foreign("nb_add_string_node_triple", 4,
                        pl_add_string_node_triple, 0);
    PL_register_foreign("nb_add_string_value_triple", 4,
                        pl_add_string_value_triple, 0);
    PL_register_foreign("nb_remove_id_triple", 4,
                        pl_remove_id_triple, 0);
    PL_register_foreign("nb_remove_string_node_triple", 4,
                        pl_remove_string_node_triple, 0);
    PL_register_foreign("nb_remove_string_value_triple", 4,
                        pl_remove_string_value_triple, 0);
    PL_register_foreign("builder_committed", 1,
                        pl_builder_committed, 0);
    PL_register_foreign("nb_commit", 2,
                        pl_builder_commit, 0);
    PL_register_foreign("nb_apply_delta", 2,
                        pl_apply_delta, 0);
    PL_register_foreign("nb_apply_diff", 2,
                        pl_apply_diff, 0);
    PL_register_foreign("node_and_value_count", 2,
                        pl_node_and_value_count, 0);
    PL_register_foreign("predicate_count", 2,
                        pl_predicate_count, 0);
    PL_register_foreign("subject_to_id", 3,
                        pl_subject_to_id, 0);
    PL_register_foreign("id_to_subject", 3,
                        pl_id_to_subject, 0);
    PL_register_foreign("predicate_to_id", 3,
                        pl_predicate_to_id, 0);
    PL_register_foreign("id_to_predicate", 3,
                        pl_id_to_predicate, 0);
    PL_register_foreign("object_node_to_id", 3,
                        pl_object_node_to_id, 0);
    PL_register_foreign("object_value_to_id", 3,
                        pl_object_value_to_id, 0);
    PL_register_foreign("id_to_object", 4,
                        pl_id_to_object, 0);
    PL_register_foreign("parent", 2,
                        pl_layer_parent, 0);
    PL_register_foreign("squash", 2,
                        pl_layer_squash, 0);
    PL_register_foreign("rollup", 1,
                        pl_layer_rollup, 0);
    PL_register_foreign("layer_addition_count", 2,
                        pl_layer_addition_count, 0);
    PL_register_foreign("layer_removal_count", 2,
                        pl_layer_removal_count, 0);
    PL_register_foreign("layer_total_addition_count", 2,
                        pl_layer_total_addition_count, 0);
    PL_register_foreign("layer_total_removal_count", 2,
                        pl_layer_total_removal_count, 0);
    PL_register_foreign("layer_total_triple_count", 2,
                        pl_layer_total_triple_count, 0);
    PL_register_foreign("num_store_blobs", 1,
                        pl_num_store_blobs, 0);
    PL_register_foreign("num_named_graph_blobs", 1,
                        pl_num_named_graph_blobs, 0);
    PL_register_foreign("num_layer_blobs", 1,
                        pl_num_layer_blobs, 0);
    PL_register_foreign("num_layer_builder_blobs", 1,
                        pl_num_layer_builder_blobs, 0);
    PL_register_foreign("layer_to_id", 2,
                        pl_layer_to_id, 0);
    PL_register_foreign("store_id_layer", 3,
                        pl_store_id_layer, 0);
    PL_register_foreign("pack_export", 3,
                        pl_pack_export, 0);
    PL_register_foreign("pack_layerids_and_parents", 2,
                        pl_pack_layerids_and_parents, 0);
    PL_register_foreign("pack_import", 3,
                        pl_pack_import, 0);
    PL_register_foreign("id_triple", 4,
                        pl_id_triple, PL_FA_NONDETERMINISTIC);
    PL_register_foreign("id_triple_addition", 4,
                        pl_id_triple_addition, PL_FA_NONDETERMINISTIC);
    PL_register_foreign("id_triple_removal", 4,
                        pl_id_triple_removal, PL_FA_NONDETERMINISTIC);
}
