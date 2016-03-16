#include "libuhuru-config.h"

#include <libuhuru/core.h>

#include "uhurup.h"
#include "confparser.h"
#include "os/dir.h"
#include "os/string.h"

#include <assert.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* old function, empty for now just to compile */
void conf_load_file(struct uhuru *uhuru, const char *filename)
{
  uhuru_log(UHURU_LOG_LIB, UHURU_LOG_LEVEL_WARNING, "conf_load_file() stub");
}

/* old function, empty for now just to compile */
void conf_load_path(struct uhuru *uhuru, const char *path)
{
  uhuru_log(UHURU_LOG_LIB, UHURU_LOG_LEVEL_WARNING, "conf_load_path() stub");
}

struct key_entry {
  const char *key;
  size_t len;
  const char **values;
};

static void key_entry_init(struct key_entry *entry, const char *key, const char **values, size_t len)
{
  int i;

  entry->key = os_strdup(key);

  entry->len = len;
  entry->values = malloc((len + 1)*sizeof(const char *));

  for(i = 0; i < len; i++)
    entry->values[i] = os_strdup(values[i]);

  entry->values[len] = NULL;
}

static void key_entry_destroy(struct key_entry *entry)
{
  const char **p;
  
  free((void *)entry->key);

  for(p = entry->values; *p != NULL; p++)
    free((void *)*p);
}

struct section_entry {
  const char *section;
  GArray *keys;
};

static void section_entry_init(struct section_entry *entry, const char *section)
{
  entry->section = os_strdup(section);

  entry->keys = g_array_new(FALSE, FALSE, sizeof(struct key_entry));
  g_array_set_clear_func(entry->keys,(GDestroyNotify)key_entry_destroy);
}

static void section_entry_destroy(struct section_entry *entry)
{
  free((void *)entry->section);

  g_array_free(entry->keys, TRUE);
}

struct uhuru_conf {
  GArray *sections;
};

struct uhuru_conf *uhuru_conf_new(void)
{
  struct uhuru_conf *conf = malloc(sizeof(struct uhuru_conf));
  
  conf->sections = g_array_new(FALSE, FALSE, sizeof(struct section_entry));
  g_array_set_clear_func(conf->sections,(GDestroyNotify)section_entry_destroy);

  return conf;
}

void uhuru_conf_free(struct uhuru_conf *conf)
{
  g_array_free(conf->sections, TRUE);

  free(conf);
}

static struct section_entry *conf_section_get(struct uhuru_conf *conf, const char *section)
{
  int i;

  for(i = 0; i < conf->sections->len; i++) {
    struct section_entry *s = &g_array_index(conf->sections, struct section_entry, i);

    if (!strcmp(s->section, section))
      return s;
  }

  return NULL;
}

static struct section_entry *conf_section_append(struct uhuru_conf *conf, const char *section)
{
  struct section_entry new_s;

  section_entry_init(&new_s, section);
  g_array_append_val(conf->sections, new_s);

  return &g_array_index(conf->sections, struct section_entry, conf->sections->len - 1);
}

#if 0
static void conf_key_insert(struct uhuru_conf *conf, const char *section, const char *key, const char **values, size_t len)
{
  struct section_entry *s = conf_section_insert(conf, section);
  int i;
  struct key_entry new_k;

  for(i = 0; i < s->keys->len; i++) {
    struct key_entry *k = &g_array_index(s->keys, struct key_entry, i);

    if (!strcmp(k->key, key))
      return;
  }

  key_entry_init(&new_k, key, values, len);
  g_array_append_val(s->keys, new_k);
}
#endif

static int conf_load_parser_cb(const char *section, const char *key, const char **values, size_t len, void *user_data)
{
  uhuru_conf_append((struct uhuru_conf *)user_data, section, key, values, len);

  return 0;
}

int uhuru_conf_load_file(struct uhuru_conf *conf, const char *path, uhuru_error **error)
{
  struct uhuru_conf_parser *cp = uhuru_conf_parser_new(path, conf_load_parser_cb, conf);

  uhuru_conf_parser_parse(cp);

  uhuru_conf_parser_free(cp);

  return 0;
}

struct conf_save_data {
  FILE *f;
  const char *p;
};

static void conf_save_fun(const char *section, const char *key, const char **values, size_t len, void *user_data)
{
  struct conf_save_data *data = (struct conf_save_data *)user_data;
  int n;

  if (data->p == NULL || strcmp(data->p, section)) {
    fprintf(data->f, "\n[%s]\n\n", section);
    data->p = section;
  }

  fprintf(data->f, "%s =", key);

  for(n = 0; n < len; n++)
    fprintf(data->f, "%s\"%s\"", (n == 0) ? " " : "; ", values[n]);

  fprintf(data->f, "\n");
}

int uhuru_conf_save_file(struct uhuru_conf *conf, const char *path, uhuru_error **error)
{
  FILE *f;
  struct conf_save_data data;

  if (!strcmp(path, "-"))
    f = stdout;
  else
    f = fopen(path, "w");

  data.f = f;
  data.p = NULL;
  uhuru_conf_apply(conf, conf_save_fun, &data);

  if (f != stdout)
    fclose(f);
  
  return 0;
}

const char **uhuru_conf_get_sections(struct uhuru_conf *conf, size_t *plen)
{
  const char **ret;
  size_t len, i;

  len = conf->sections->len;
  ret = malloc((len + 1)*sizeof(const char *));

  for(i = 0; i < len; i++) {
    struct section_entry *s = &g_array_index(conf->sections, struct section_entry, i);

    ret[i] = os_strdup(s->section);
  }

  ret[len] = NULL;

  if (plen != NULL)
    *plen = len;

  return ret;
}

const char **uhuru_conf_get_keys(struct uhuru_conf *conf, const char *section, size_t *plen)
{
  struct section_entry *s;
  const char **ret;
  size_t len, i;

  s = conf_section_get(conf, section);
  if (s == NULL)
    return NULL;

  len = s->keys->len;
  ret = malloc((len + 1)*sizeof(const char *));

  for(i = 0; i < len; i++) {
    struct key_entry *k = &g_array_index(s->keys, struct key_entry, i);

    ret[i] = os_strdup(k->key);
  }

  ret[len] = NULL;

  if (plen != NULL)
    *plen = len;

  return ret;
}

void uhuru_conf_append(struct uhuru_conf *conf, const char *section, const char *key, const char **values, size_t len)
{
  struct section_entry *s;
  struct key_entry new_k;

  s = conf_section_get(conf, section);
  if (s == NULL)
    s= conf_section_append(conf, section);

  key_entry_init(&new_k, key, values, len);
  g_array_append_val(s->keys, new_k);
}

void uhuru_conf_apply(struct uhuru_conf *conf, uhuru_conf_fun_t fun, void *user_data)
{
  int i;

  for(i = 0; i < conf->sections->len; i++) {
    struct section_entry *s = &g_array_index(conf->sections, struct section_entry, i);
    int j;

    for(j = 0; j < s->keys->len; j++) {
      struct key_entry *k = &g_array_index(s->keys, struct key_entry, j);
	
      (*fun)(s->section, k->key, k->values, k->len, user_data); 
    }     
  }
}

