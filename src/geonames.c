#include "geonames.h"

#define DEFAULT_BUFFER_SIZE 4096

#define GEONAMES_NAME_DEFAULT_LENGTH 255
#define GEONAMES_ISO_LANGUAGE_DEFAULT_LENGTH 10
#define GEONAMES_FEATURE_CODE_DEFAULT_LENGTH 5
#define GEONAMES_COUNTRY_CODE_DEFAULT_LENGTH 3
#define GEONAMES_ADMIN1_CODE_DEFAULT_LENGTH 32
#define GEONAMES_ADMIN2_CODE_DEFAULT_LENGTH 64
#define GEONAMES_ADMIN3_CODE_DEFAULT_LENGTH 32
#define GEONAMES_ADMIN4_CODE_DEFAULT_LENGTH 32
#define GEONAMES_POSTAL_CODE_DEFAULT_LENGTH 64


#define GEONAMES_POSTAL_ADMIN1_IDS_DEFAULT_LENGTH 10
#define GEONAMES_POSTAL_ADMIN2_IDS_DEFAULT_LENGTH 20
#define GEONAMES_POSTAL_ADMIN3_IDS_DEFAULT_LENGTH 20

/* To save on malloc calls, create just one of these and call geoname_clear 
*  (which does not deallocate) and geoname_deserialize
*/
geoname_t *geoname_new(void) {
    geoname_t *self = malloc(sizeof(geoname_t));
    self->name = char_array_new_size(GEONAMES_NAME_DEFAULT_LENGTH);
    self->canonical = char_array_new_size(GEONAMES_NAME_DEFAULT_LENGTH);

    self->iso_language = char_array_new_size(GEONAMES_ISO_LANGUAGE_DEFAULT_LENGTH);

    self->feature_code = char_array_new_size(GEONAMES_FEATURE_CODE_DEFAULT_LENGTH);
    self->country_code = char_array_new_size(GEONAMES_COUNTRY_CODE_DEFAULT_LENGTH);

    self->admin1_code = char_array_new_size(GEONAMES_ADMIN1_CODE_DEFAULT_LENGTH);
    self->admin2_code = char_array_new_size(GEONAMES_ADMIN2_CODE_DEFAULT_LENGTH);
    self->admin3_code = char_array_new_size(GEONAMES_ADMIN3_CODE_DEFAULT_LENGTH);
    self->admin4_code = char_array_new_size(GEONAMES_ADMIN4_CODE_DEFAULT_LENGTH);

    return self;
}

void geoname_clear(geoname_t *self) {
    char_array_clear(self->name);
    char_array_clear(self->canonical);
    char_array_clear(self->iso_language);
    char_array_clear(self->country_code);
    char_array_clear(self->admin1_code);
    char_array_clear(self->admin2_code);
    char_array_clear(self->admin3_code);
    char_array_clear(self->admin4_code);
}

void geoname_destroy(geoname_t *self) {
    if (!self)
        return;

    if (self->name)
        char_array_destroy(self->name);

    if (self->canonical)
        char_array_destroy(self->canonical);

    if (self->iso_language)
        char_array_destroy(self->iso_language);

    if (self->feature_code)
        char_array_destroy(self->feature_code);

    if (self->country_code)
        char_array_destroy(self->country_code);

    if (self->admin1_code)
        char_array_destroy(self->admin1_code);

    if (self->admin2_code)
        char_array_destroy(self->admin2_code);

    if (self->admin3_code)
        char_array_destroy(self->admin3_code);

    if (self->admin4_code)
        char_array_destroy(self->admin4_code);

    free(self);
}

static char_array *read_string(char_array *str, size_t len, buffer_t *buffer) {
    char_array_clear(str);
    char_array_cat_len(str, buffer->data->a + buffer->offset, len);
    buffer->offset += len;
    return str;
}

static bool bytes_reader(cmp_ctx_t *ctx, void *data, size_t size) {
    buffer_t *buffer = ctx->buf;
    if (buffer->offset + size > char_array_len(buffer->data))
        return false;
    memcpy(data, buffer->data->a + buffer->offset, size);
    buffer->offset += size;
    return true;
}

static size_t bytes_writer(cmp_ctx_t *ctx, const void *data, size_t count) {
    buffer_t *buffer = (buffer_t *)ctx->buf;
    char_array_cat_len(buffer->data, (char *)data, count);
    buffer->offset += count;
    return count;
}

bool cmp_read_str_size_or_nil(cmp_ctx_t *ctx, char_array **str, uint32_t *size) {
    cmp_object_t obj;

    if (!cmp_read_object(ctx, &obj))
        return false;

    if (cmp_object_is_str(&obj)) {
        *size = obj.as.str_size;
        *str = read_string(*str, *size, ctx->buf);
    } else if (cmp_object_is_nil(&obj)) {
        *size = 0;
        char_array_clear(*str);
    }

    return true;
}

bool cmp_write_str_or_nil(cmp_ctx_t *ctx, char_array *str) {
    if (str != NULL && char_array_len(str) > 0) {
        return cmp_write_str(ctx, str->a, char_array_len(str));
    } else {
        return cmp_write_nil(ctx);
    }
}

bool cmp_write_uint_vector(cmp_ctx_t *ctx, uint32_array *array) {
    size_t n = array->n;
    if (!cmp_write_array(ctx, n))
        return false;

    for (size_t i = 0; i < n; i++) {
        if (!cmp_write_uint(ctx, array->a[i]))
            return false;
    }
    return true;
}

bool geoname_deserialize(geoname_t *self, char_array *str) {
    cmp_ctx_t ctx;
    buffer_t buffer = (buffer_t){str, 0};
    cmp_init(&ctx, &buffer, bytes_reader, bytes_writer);

    uint32_t len;

    if (!cmp_read_uint(&ctx, &self->geonames_id))
        return false;

    if (!cmp_read_str_size_or_nil(&ctx, &self->name, &len))
        return false;

    if (!cmp_read_str_size_or_nil(&ctx, &self->canonical, &len))
        return false;

    if (!cmp_read_uint(&ctx, &self->type))
        return false;

    if (!cmp_read_str_size_or_nil(&ctx, &self->iso_language, &len))
        return false;

    if (!cmp_read_bool(&ctx, &self->is_preferred_name))
        return false;

    if (!cmp_read_uint(&ctx, &self->population))
        return false;

    if (!cmp_read_double(&ctx, &self->latitude))
        return false;

    if (!cmp_read_double(&ctx, &self->longitude))
        return false;

    if (!cmp_read_str_size_or_nil(&ctx, &self->feature_code, &len))
        return false;

    if (!cmp_read_str_size_or_nil(&ctx, &self->country_code, &len))
        return false;

    if (!cmp_read_str_size_or_nil(&ctx, &self->admin1_code, &len))
        return false;

    if (!cmp_read_uint(&ctx, &self->admin1_geonames_id))
        return false;

    if (!cmp_read_str_size_or_nil(&ctx, &self->admin2_code, &len))
        return false;

    if (!cmp_read_uint(&ctx, &self->admin2_geonames_id))
        return false;

    if (!cmp_read_str_size_or_nil(&ctx, &self->admin3_code, &len))
        return false;

    if (!cmp_read_uint(&ctx, &self->admin3_geonames_id))
        return false;

    if (!cmp_read_str_size_or_nil(&ctx, &self->admin4_code, &len))
        return false;

    if (!cmp_read_uint(&ctx, &self->admin4_geonames_id))
        return false;

    return true;
}

bool geoname_serialize(geoname_t *self, char_array *str) {
    cmp_ctx_t ctx;
    buffer_t buffer = (buffer_t){str, 0};
    cmp_init(&ctx, &buffer, bytes_reader, bytes_writer);

    if (!cmp_write_uint(&ctx, self->geonames_id) ||
        !cmp_write_str_or_nil(&ctx, self->name) ||
        !cmp_write_str_or_nil(&ctx, self->canonical) ||
        !cmp_write_uint(&ctx, self->type) ||
        !cmp_write_str_or_nil(&ctx, self->iso_language) ||
        !cmp_write_bool(&ctx, self->is_preferred_name) ||
        !cmp_write_uint(&ctx, self->population) ||
        !cmp_write_double(&ctx, self->latitude) ||
        !cmp_write_double(&ctx, self->longitude) ||
        !cmp_write_str_or_nil(&ctx, self->feature_code) ||
        !cmp_write_str_or_nil(&ctx, self->country_code) ||
        !cmp_write_str_or_nil(&ctx, self->admin1_code) ||
        !cmp_write_uint(&ctx, self->admin1_geonames_id) ||
        !cmp_write_str_or_nil(&ctx, self->admin2_code) ||
        !cmp_write_uint(&ctx, self->admin2_geonames_id) ||
        !cmp_write_str_or_nil(&ctx, self->admin3_code) ||
        !cmp_write_uint(&ctx, self->admin3_geonames_id) ||
        !cmp_write_str_or_nil(&ctx, self->admin4_code) ||
        !cmp_write_uint(&ctx, self->admin4_geonames_id)
        ) { return false; }

    return true;
}

void geoname_print(geoname_t *self) {
    printf("geonames_id: %d\n", self->geonames_id);
    printf("name: %s\n", char_array_get_string(self->name));
    printf("canonical: %s\n", char_array_get_string(self->canonical));
    printf("type: %d\n", self->type);
    printf("iso_language: %s\n", char_array_get_string(self->iso_language));
    printf("is_preferred: %d\n", self->is_preferred_name);
    printf("population: %d\n", self->population);
    printf("latitude: %f\n", self->latitude);
    printf("longitude: %f\n", self->longitude);
    printf("feature_code: %s\n", char_array_get_string(self->feature_code));
    printf("country_code: %s\n", char_array_get_string(self->country_code));
    printf("admin1_code: %s\n", char_array_get_string(self->admin1_code));
    printf("admin1_geonames_id: %d\n", self->admin1_geonames_id);
    printf("admin2_code: %s\n", char_array_get_string(self->admin2_code));
    printf("admin2_geonames_id: %d\n", self->admin2_geonames_id);
    printf("admin3_code: %s\n", char_array_get_string(self->admin3_code));
    printf("admin3_geonames_id: %d\n", self->admin3_geonames_id);
    printf("admin4_code: %s\n", char_array_get_string(self->admin4_code));
    printf("admin4_geonames_id: %d\n", self->admin4_geonames_id);
}

gn_postal_code_t *gn_postal_code_new(void) {
    gn_postal_code_t *self = malloc(sizeof(gn_postal_code_t));
    self->postal_code = char_array_new_size(GEONAMES_POSTAL_CODE_DEFAULT_LENGTH);
    self->country_code = char_array_new_size(GEONAMES_COUNTRY_CODE_DEFAULT_LENGTH);
    self->containing_geoname = char_array_new_size(GEONAMES_NAME_DEFAULT_LENGTH);

    self->admin1_ids = uint32_array_new_size(GEONAMES_POSTAL_ADMIN1_IDS_DEFAULT_LENGTH);
    self->admin2_ids = uint32_array_new_size(GEONAMES_POSTAL_ADMIN2_IDS_DEFAULT_LENGTH);
    self->admin3_ids = uint32_array_new_size(GEONAMES_POSTAL_ADMIN3_IDS_DEFAULT_LENGTH);

    return self;
}

void gn_postal_code_clear(gn_postal_code_t *self) {
    char_array_clear(self->postal_code);
    char_array_clear(self->country_code);
    char_array_clear(self->containing_geoname);
    uint32_array_clear(self->admin1_ids);
    uint32_array_clear(self->admin2_ids);
    uint32_array_clear(self->admin3_ids);
}

void gn_postal_code_destroy(gn_postal_code_t *self) {
    if (!self)
        return;

    if (self->postal_code)
        char_array_destroy(self->postal_code);

    if (self->country_code)
        char_array_destroy(self->country_code);

    if (self->containing_geoname)
        char_array_destroy(self->containing_geoname);

    if (self->admin1_ids)
        uint32_array_destroy(self->admin1_ids);

    if (self->admin2_ids)
        uint32_array_destroy(self->admin2_ids);

    if (self->admin3_ids)
        uint32_array_destroy(self->admin3_ids);

    free(self);
}

bool gn_postal_code_deserialize(gn_postal_code_t *self, char_array *str) {
    cmp_ctx_t ctx;
    buffer_t buffer = (buffer_t){str, 0};
    cmp_init(&ctx, &buffer, bytes_reader, bytes_writer);

    uint32_t len;

    if (!cmp_read_str_size_or_nil(&ctx, &self->postal_code, &len))
        return false;

    if (!cmp_read_str_size_or_nil(&ctx, &self->country_code, &len))
        return false;

    if (!cmp_read_bool(&ctx, &self->have_containing_geoname))
        return false;

    if (self->have_containing_geoname) {
        if (!cmp_read_str_size_or_nil(&ctx, &self->containing_geoname, &len))
            return false;

        if (!cmp_read_uint(&ctx, &self->containing_geonames_id))
            return false;
    }

    uint32_t array_size;

    if (!cmp_read_array(&ctx, &array_size))
        return false;

    if (array_size > 0) {
        uint32_t admin1_id;
        for ( ;array_size; array_size--) {
            if (!cmp_read_uint(&ctx, &admin1_id))
                return false;
            uint32_array_push(self->admin1_ids, admin1_id);
        }
    }

    if (!cmp_read_array(&ctx, &array_size))
        return false;

    if (array_size > 0) {
        uint32_t admin2_id;
        for (; array_size > 0; array_size--) {
            if (!cmp_read_uint(&ctx, &admin2_id))
                return false;
            uint32_array_push(self->admin2_ids, admin2_id);
        }
    }

    if (!cmp_read_array(&ctx, &array_size))
        return false;

    if (array_size > 0) {
        uint32_t admin3_id;
        for (; array_size > 0; array_size--) {
            if (!cmp_read_uint(&ctx, &admin3_id))
                return false;
            uint32_array_push(self->admin3_ids, admin3_id);
        }
    }

    return true;
}

bool gn_postal_code_serialize(gn_postal_code_t *self, char_array *str) {
    cmp_ctx_t ctx;
    buffer_t buffer = (buffer_t){str, 0};
    cmp_init(&ctx, &buffer, bytes_reader, bytes_writer);

    if (!cmp_write_str_or_nil(&ctx, self->postal_code) ||
        !cmp_write_str_or_nil(&ctx, self->country_code) ||
        !cmp_write_bool(&ctx, self->have_containing_geoname) ||
        (self->have_containing_geoname &&
            (!cmp_write_str_or_nil(&ctx, self->containing_geoname) ||
             !cmp_write_uint(&ctx, self->containing_geonames_id)
            )
        ) ||
        !cmp_write_uint_vector(&ctx, self->admin1_ids) ||
        !cmp_write_uint_vector(&ctx, self->admin2_ids) ||
        !cmp_write_uint_vector(&ctx, self->admin3_ids)
        ) { return false; }

    return true;
}

void gn_postal_code_print(gn_postal_code_t *self) {
    int i;
    printf("postal_code: %s\n", char_array_get_string(self->postal_code));
    printf("country_code: %s\n", char_array_get_string(self->country_code));
    printf("have_containing_geoname: %d\n", self->have_containing_geoname);
    if (self->have_containing_geoname) {
        printf("containing_geoname: %s\n", char_array_get_string(self->containing_geoname));
        printf("containing_geonames_id: %d\n", self->containing_geonames_id);
    }
    printf("admin1_ids: [ ");
    for (i = 0; i < self->admin1_ids->n; i++) {
        printf("%d ", self->admin1_ids->a[i]);
    }
    printf("]\n");
    printf("admin2_ids: [ ");
    for (i = 0; i < self->admin2_ids->n; i++) {
        printf("%d ", self->admin2_ids->a[i]);
    }
    printf("]\n");
    printf("admin3_ids: [ ");
    for (i = 0; i < self->admin3_ids->n; i++) {
        printf("%d ", self->admin3_ids->a[i]);
    }
    printf("]\n");
}


void geonames_generic_serialize(geonames_generic_t gn) {

}

geonames_generic_t *geonames_generic_deserialize(geonames_generic_t gn) {

}