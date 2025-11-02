#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "zend_exceptions.h"
#include "zend_interfaces.h"
#include "SAPI.h"

#define PHP_ZDUMP_VERSION "1.0.0"
#define PHP_ZDUMP_EXTNAME "zdump"

// Module globals
ZEND_BEGIN_MODULE_GLOBALS(zdump)
    zend_long max_depth;
    zend_long max_children;
    zend_bool enable_colors;
    zend_bool html_mode;
    zend_bool light_mode;
ZEND_END_MODULE_GLOBALS(zdump)

ZEND_DECLARE_MODULE_GLOBALS(zdump)

#define ZDUMP_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(zdump, v)

// ANSI Color codes
#define COLOR_RESET     "\x1b[0m"
#define COLOR_BOLD      "\x1b[1m"
#define COLOR_DIM       "\x1b[2m"
#define COLOR_RED       "\x1b[31m"
#define COLOR_GREEN     "\x1b[32m"
#define COLOR_YELLOW    "\x1b[33m"
#define COLOR_BLUE      "\x1b[34m"
#define COLOR_MAGENTA   "\x1b[35m"
#define COLOR_CYAN      "\x1b[36m"
#define COLOR_GRAY      "\x1b[90m"
#define COLOR_BRIGHT_BLUE   "\x1b[94m"
#define COLOR_BRIGHT_GREEN  "\x1b[92m"

// HTML Color codes - Dark mode (One Dark Pro theme)
#define HTML_DARK_RESET      "</span>"
#define HTML_DARK_BOLD       "<span style='font-weight:bold'>"
#define HTML_DARK_DIM        "<span style='opacity:0.6'>"
#define HTML_DARK_RED        "<span style='color:#f48771'>"
#define HTML_DARK_GREEN      "<span style='color:#89d185'>"
#define HTML_DARK_YELLOW     "<span style='color:#e5c07b'>"
#define HTML_DARK_BLUE       "<span style='color:#61afef'>"
#define HTML_DARK_MAGENTA    "<span style='color:#c678dd'>"
#define HTML_DARK_CYAN       "<span style='color:#56b6c2'>"
#define HTML_DARK_GRAY       "<span style='color:#5c6370'>"
#define HTML_DARK_BRIGHT_BLUE    "<span style='color:#61afef;font-weight:bold'>"
#define HTML_DARK_BRIGHT_GREEN   "<span style='color:#89d185;font-weight:bold'>"

// HTML Color codes - Light mode
#define HTML_LIGHT_RESET      "</span>"
#define HTML_LIGHT_BOLD       "<span style='font-weight:bold'>"
#define HTML_LIGHT_DIM        "<span style='opacity:0.6'>"
#define HTML_LIGHT_RED        "<span style='color:#e45649'>"
#define HTML_LIGHT_GREEN      "<span style='color:#50a14f'>"
#define HTML_LIGHT_YELLOW     "<span style='color:#c18401'>"
#define HTML_LIGHT_BLUE       "<span style='color:#4078f2'>"
#define HTML_LIGHT_MAGENTA    "<span style='color:#a626a4'>"
#define HTML_LIGHT_CYAN       "<span style='color:#0184bc'>"
#define HTML_LIGHT_GRAY       "<span style='color:#a0a1a7'>"
#define HTML_LIGHT_BRIGHT_BLUE    "<span style='color:#4078f2;font-weight:bold'>"
#define HTML_LIGHT_BRIGHT_GREEN   "<span style='color:#50a14f;font-weight:bold'>"

// Color helper functions
static const char* get_reset() {
    if (!ZDUMP_G(html_mode)) return COLOR_RESET;
    return ZDUMP_G(light_mode) ? HTML_LIGHT_RESET : HTML_DARK_RESET;
}
static const char* get_bold() {
    if (!ZDUMP_G(html_mode)) return COLOR_BOLD;
    return ZDUMP_G(light_mode) ? HTML_LIGHT_BOLD : HTML_DARK_BOLD;
}
static const char* get_dim() {
    if (!ZDUMP_G(html_mode)) return COLOR_DIM;
    return ZDUMP_G(light_mode) ? HTML_LIGHT_DIM : HTML_DARK_DIM;
}
static const char* get_red() {
    if (!ZDUMP_G(html_mode)) return COLOR_RED;
    return ZDUMP_G(light_mode) ? HTML_LIGHT_RED : HTML_DARK_RED;
}
static const char* get_green() {
    if (!ZDUMP_G(html_mode)) return COLOR_GREEN;
    return ZDUMP_G(light_mode) ? HTML_LIGHT_GREEN : HTML_DARK_GREEN;
}
static const char* get_yellow() {
    if (!ZDUMP_G(html_mode)) return COLOR_YELLOW;
    return ZDUMP_G(light_mode) ? HTML_LIGHT_YELLOW : HTML_DARK_YELLOW;
}
static const char* get_blue() {
    if (!ZDUMP_G(html_mode)) return COLOR_BLUE;
    return ZDUMP_G(light_mode) ? HTML_LIGHT_BLUE : HTML_DARK_BLUE;
}
static const char* get_magenta() {
    if (!ZDUMP_G(html_mode)) return COLOR_MAGENTA;
    return ZDUMP_G(light_mode) ? HTML_LIGHT_MAGENTA : HTML_DARK_MAGENTA;
}
static const char* get_cyan() {
    if (!ZDUMP_G(html_mode)) return COLOR_CYAN;
    return ZDUMP_G(light_mode) ? HTML_LIGHT_CYAN : HTML_DARK_CYAN;
}
static const char* get_gray() {
    if (!ZDUMP_G(html_mode)) return COLOR_GRAY;
    return ZDUMP_G(light_mode) ? HTML_LIGHT_GRAY : HTML_DARK_GRAY;
}
static const char* get_bright_blue() {
    if (!ZDUMP_G(html_mode)) return COLOR_BRIGHT_BLUE;
    return ZDUMP_G(light_mode) ? HTML_LIGHT_BRIGHT_BLUE : HTML_DARK_BRIGHT_BLUE;
}
static const char* get_bright_green() {
    if (!ZDUMP_G(html_mode)) return COLOR_BRIGHT_GREEN;
    return ZDUMP_G(light_mode) ? HTML_LIGHT_BRIGHT_GREEN : HTML_DARK_BRIGHT_GREEN;
}

static int is_browser_context() {
    const char *sapi_name = sapi_module.name;
    return strcmp(sapi_name, "cli") != 0;
}

static int should_use_colors() {
    if (!ZDUMP_G(enable_colors)) return 0;
    if (is_browser_context()) {
        ZDUMP_G(html_mode) = 1;
        return 1;
    }
    if (isatty(STDOUT_FILENO)) {
        ZDUMP_G(html_mode) = 0;
        return 1;
    }
    return 0;
}

static void zdump_value(zval *val, int depth);

static const char* get_type_name(zval *val) {
    switch (Z_TYPE_P(val)) {
        case IS_NULL: return "null";
        case IS_TRUE:
        case IS_FALSE: return "bool";
        case IS_LONG: return "int";
        case IS_DOUBLE: return "float";
        case IS_STRING: return "string";
        case IS_ARRAY: return "array";
        case IS_OBJECT: return "object";
        case IS_RESOURCE: return "resource";
        default: return "unknown";
    }
}

static void zdump_array(zval *val, int depth) {
    HashTable *ht = Z_ARRVAL_P(val);
    zend_long idx;
    zend_string *key;
    zval *data;
    int use_colors = should_use_colors();
    int count = 0;
    int max_children = ZDUMP_G(max_children);
    
    ZEND_HASH_FOREACH_KEY_VAL(ht, idx, key, data) {
        if (count >= max_children) {
            for (int i = 0; i <= depth; i++) php_printf("  ");
            if (use_colors) {
                php_printf("%s... %ld more elements%s\n", get_dim(), zend_hash_num_elements(ht) - max_children, get_reset());
            } else {
                php_printf("... %ld more elements\n", zend_hash_num_elements(ht) - max_children);
            }
            break;
        }

        for (int i = 0; i <= depth; i++) {
            php_printf("  ");
        }

        if (key) {
            if (use_colors) {
                php_printf("%s[\"%s\"]%s => ", get_cyan(), ZSTR_VAL(key), get_reset());
            } else {
                php_printf("[\"%s\"] => ", ZSTR_VAL(key));
            }
        } else {
            if (use_colors) {
                php_printf("%s[%ld]%s => ", get_cyan(), idx, get_reset());
            } else {
                php_printf("[%ld] => ", idx);
            }
        }

        zdump_value(data, depth + 1);
        php_printf("\n");
        
        count++;
    } ZEND_HASH_FOREACH_END();
}

static void zdump_object(zval *val, int depth) {
    zend_object *obj = Z_OBJ_P(val);
    zend_class_entry *ce = obj->ce;
    HashTable *props = obj->handlers->get_properties(obj);
    int use_colors = should_use_colors();
    
    if (!props) return;
    
    zend_string *key;
    zval *data;
    int count = 0;
    int max_children = ZDUMP_G(max_children);
    
    ZEND_HASH_FOREACH_STR_KEY_VAL(props, key, data) {
        if (count >= max_children) {
            for (int i = 0; i <= depth; i++) php_printf("  ");
            if (use_colors) {
                php_printf("%s... %d more properties%s\n", get_dim(), zend_hash_num_elements(props) - max_children, get_reset());
            } else {
                php_printf("... %d more properties\n", zend_hash_num_elements(props) - max_children);
            }
            break;
        }

        if (!key) continue;

        for (int i = 0; i <= depth; i++) {
            php_printf("  ");
        }

        const char *prop_name = ZSTR_VAL(key);
        const char *visibility = "public";
        const char *vis_color;

        if (prop_name[0] == '\0') {
            if (strstr(prop_name, "*")) {
                visibility = "protected";
                vis_color = get_yellow();
            } else {
                visibility = "private";
                vis_color = get_red();
            }
        } else {
            vis_color = get_green();
        }

        if (use_colors) {
            php_printf("%s%s%s %s$%s%s => ",
                vis_color, visibility, get_reset(),
                get_cyan(), prop_name, get_reset());
        } else {
            php_printf("%s $%s => ", visibility, prop_name);
        }
        
        zdump_value(data, depth + 1);
        php_printf("\n");
        
        count++;
    } ZEND_HASH_FOREACH_END();
}

static void zdump_value(zval *val, int depth) {
    int use_colors = should_use_colors();
    int max_depth = ZDUMP_G(max_depth);
    
    if (depth > max_depth) {
        if (use_colors) {
            php_printf("%s*DEPTH LIMIT*%s", get_red(), get_reset());
        } else {
            php_printf("*DEPTH LIMIT*");
        }
        return;
    }

    switch (Z_TYPE_P(val)) {
        case IS_NULL:
            if (use_colors) {
                php_printf("%snull%s", get_gray(), get_reset());
            } else {
                php_printf("null");
            }
            break;

        case IS_TRUE:
            if (use_colors) {
                php_printf("%strue%s %s(bool)%s", get_green(), get_reset(), get_blue(), get_reset());
            } else {
                php_printf("true (bool)");
            }
            break;

        case IS_FALSE:
            if (use_colors) {
                php_printf("%sfalse%s %s(bool)%s", get_red(), get_reset(), get_blue(), get_reset());
            } else {
                php_printf("false (bool)");
            }
            break;

        case IS_LONG:
            if (use_colors) {
                php_printf("%s%ld%s %s(int)%s", get_cyan(), Z_LVAL_P(val), get_reset(), get_blue(), get_reset());
            } else {
                php_printf("%ld (int)", Z_LVAL_P(val));
            }
            break;

        case IS_DOUBLE:
            if (use_colors) {
                php_printf("%s%.14g%s %s(float)%s", get_cyan(), Z_DVAL_P(val), get_reset(), get_blue(), get_reset());
            } else {
                php_printf("%.14g (float)", Z_DVAL_P(val));
            }
            break;

        case IS_STRING:
            if (use_colors) {
                php_printf("%s\"%s\"%s %s(string)%s",
                    get_yellow(), Z_STRVAL_P(val), get_reset(),
                    get_blue(), get_reset());
            } else {
                php_printf("\"%s\" (string)", Z_STRVAL_P(val));
            }
            break;
            
        case IS_ARRAY: {
            zend_long count = zend_hash_num_elements(Z_ARRVAL_P(val));
            if (use_colors) {
                php_printf("%sarray%s%s(%ld)%s {",
                    get_bold(), get_reset(), get_dim(), count, get_reset());
            } else {
                php_printf("array(%ld) {", count);
            }

            if (count > 0) {
                php_printf("\n");
                zdump_array(val, depth);
                for (int i = 0; i < depth; i++) php_printf("  ");
            }
            php_printf("}");
            break;
        }

        case IS_OBJECT: {
            zend_class_entry *ce = Z_OBJCE_P(val);
            if (use_colors) {
                php_printf("%sobject%s(%s%s%s) {",
                    get_magenta(), get_reset(),
                    get_bright_blue(), ZSTR_VAL(ce->name), get_reset());
            } else {
                php_printf("object(%s) {", ZSTR_VAL(ce->name));
            }

            php_printf("\n");
            zdump_object(val, depth);
            for (int i = 0; i < depth; i++) php_printf("  ");
            php_printf("}");
            break;
        }

        case IS_RESOURCE: {
            const char *type_name = zend_rsrc_list_get_rsrc_type(Z_RES_P(val));
            if (use_colors) {
                php_printf("%sresource%s(%ld) of type (%s)",
                    get_magenta(), get_reset(), Z_RES_HANDLE_P(val), type_name ? type_name : "Unknown");
            } else {
                php_printf("resource(%ld) of type (%s)",
                    Z_RES_HANDLE_P(val), type_name ? type_name : "Unknown");
            }
            break;
        }
            
        default:
            php_printf("unknown type");
    }
}

PHP_FUNCTION(zdump)
{
    zval *arg;
    int use_colors = should_use_colors();

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(arg)
    ZEND_PARSE_PARAMETERS_END();

    zend_execute_data *call = EG(current_execute_data);
    const char *filename = zend_get_executed_filename();
    uint32_t lineno = zend_get_executed_lineno();

    const char *short_filename = strrchr(filename, '/');
    if (short_filename) {
        short_filename++; 
    } else {
        short_filename = filename;
    }

    php_printf("\n");
    if (use_colors) {
        php_printf("%s%s:%d%s\n",
            get_dim(),
            short_filename,
            lineno,
            get_reset());
        php_printf("%s╭─ ZDUMP ─────────────────────────────────╮%s\n",
            get_bright_blue(), get_reset());
    } else {
        php_printf("%s:%d\n", short_filename, lineno);
        php_printf("=== ZDUMP ===\n");
    }

    zdump_value(arg, 0);

    php_printf("\n");
    if (use_colors) {
        php_printf("%s╰──────────────────────────────────────────╯%s\n",
            get_bright_blue(), get_reset());
    } else {
        php_printf("=============\n");
    }
    php_printf("\n");
}

PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("zdump.max_depth", "10", PHP_INI_ALL, OnUpdateLong, max_depth, zend_zdump_globals, zdump_globals)
    STD_PHP_INI_ENTRY("zdump.max_children", "128", PHP_INI_ALL, OnUpdateLong, max_children, zend_zdump_globals, zdump_globals)
    STD_PHP_INI_ENTRY("zdump.enable_colors", "1", PHP_INI_ALL, OnUpdateBool, enable_colors, zend_zdump_globals, zdump_globals)
    STD_PHP_INI_ENTRY("zdump.light_mode", "1", PHP_INI_ALL, OnUpdateBool, light_mode, zend_zdump_globals, zdump_globals)
PHP_INI_END()

static const zend_function_entry zdump_functions[] = {
    PHP_FE(zdump, NULL)
    PHP_FE_END
};

PHP_MINIT_FUNCTION(zdump)
{
    REGISTER_INI_ENTRIES();
    return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(zdump)
{
    UNREGISTER_INI_ENTRIES();
    return SUCCESS;
}

PHP_MINFO_FUNCTION(zdump)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "zdump support", "enabled");
    php_info_print_table_row(2, "Version", PHP_ZDUMP_VERSION);
    php_info_print_table_end();
    
    DISPLAY_INI_ENTRIES();
}

zend_module_entry zdump_module_entry = {
    STANDARD_MODULE_HEADER,
    PHP_ZDUMP_EXTNAME,
    zdump_functions,
    PHP_MINIT(zdump),
    PHP_MSHUTDOWN(zdump),
    NULL,
    NULL,
    PHP_MINFO(zdump),
    PHP_ZDUMP_VERSION,
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_ZDUMP
ZEND_GET_MODULE(zdump)
#endif
