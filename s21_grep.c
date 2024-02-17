#include <getopt.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static const int OPTIONS_END = -1;
static const int PATTERNS_INIT = 128;
static const int PATTERNS_ADD = 128;
static const int BUFFER_INIT = 128;
static const char *FOPEN_READ = "r";
static const int BUFFER_MULT = 10;

struct Patterns {
  char **data;
  regex_t *reg_data;
  size_t cur_size;
  size_t max_size;
};
typedef struct Patterns Patterns;

struct Options {
  bool e;
  bool i;
  bool v;
  bool c;
  bool l;
  bool n;
  bool h;
  bool s;

  bool f;
  bool o;
  struct Patterns patts;
  size_t file_count;
};
typedef struct Options Options;
static const char SHORTOPTIONS[] = "e:f:isvnholc";
static struct option LONGOPTIONS[] = {
    {"regexp", required_argument, NULL,
     'e'},  // использовать ШАБЛОНЫ для поиска
    {"file", required_argument, NULL, 'f'},  // брать ШАБЛОНЫ из ФАЙЛа
    {"ignore-case", no_argument, NULL, 'i'},  // игнорировать различие регистра
    {"no-messages", no_argument, NULL,
     's'},  // не показывать сообщения об ошибках
    {"invert-match", no_argument, NULL, 'v'},  // выбирать не подходящие строки
    {"line-number", no_argument, NULL,
     'n'},  // печатать номер строки вместе с выходными строками
    {"no-filename", no_argument, NULL, 'h'},  // не начинать вывод с имени файла
    {"only-matching", no_argument, NULL,
     'o'},  // показывать только совпавшие непустые части строк
    {"file-witch-matches", no_argument, NULL,
     'l'},  // печатать только имена ФАЙЛОВ с выбранными строками
    {"count", no_argument, NULL,
     'c'},  // печатать только количество выбранных строк на ФАЙЛ
    {0, 0, 0, 0}};

/*функция выделения и проверки памяти*/
void *safe_malloc(const size_t size) {
  void *ret_ptr = malloc(size);
  if (ret_ptr == NULL) {
    printf("ERROR");
    exit(1);
  }
  return ret_ptr;
}
/*функция добавления памяти*/
void *safe_realloc(void *buffer, const size_t size) {
  void *ret_ptr = realloc(buffer, size);
  if (ret_ptr == NULL) {
    printf("ERROR no realloc\n");
    exit(1);
  }
  return ret_ptr;
}
/*функция добавления шаблонов*/
static void patterns_add(Patterns *const patts, const char *const patt) {
  if (patts->cur_size == patts->max_size) {
    patts->max_size += PATTERNS_ADD;
    patts->data = safe_realloc(patts->data, patts->max_size * sizeof(char *));
  }
  patts->data[patts->cur_size] =
      safe_malloc(sizeof(char) * strlen(patt) + sizeof(char));
  strcpy(patts->data[patts->cur_size++], patt);
}
/*фкнкция открытия файла*/
FILE *safe_fopen(const char *filename, const char *mode) {
  FILE *file = fopen(filename, mode);
  if (file == NULL) {
    perror("Error opening file");
    exit(EXIT_FAILURE);
  }
  return file;
}
/*функция создания буфера*/
static void buffer_file(FILE *file, char *buffer) {
  size_t size = 0;
  size_t max_size = BUFFER_INIT;
  char simbol = fgetc(file);
  while (!feof(file)) {
    buffer[size++] = simbol;
    simbol = fgetc(file);
    if (size == max_size) {
      max_size *= BUFFER_MULT;
      buffer = safe_realloc(buffer, max_size * sizeof(char));
    }
  }
  buffer[size] = '\0';
}
/*функция разбора строк на лексемы*/
static void patterns_add_from_string(Patterns *const patts,
                                     const char *const str) {
  char *temp_patts = safe_malloc((sizeof(char)) * (strlen(str) + 1));
  strcpy(temp_patts, str);
  char *token = strtok(temp_patts, "\n");
  while (token != NULL) {
    patterns_add(patts, token);
    token = strtok(NULL, "\n");
  }
  free(temp_patts);
}
/*функция разбора файла на лексемы*/
static void patterns_add_from_file(Patterns *const patts,
                                   char *const filename) {
  FILE *file = safe_fopen(filename, FOPEN_READ);
  char *buffer = safe_malloc(sizeof(char) * BUFFER_INIT);
  buffer_file(file, buffer);
  patterns_add_from_string(patts, buffer);
  free(buffer);
  fclose(file);
}
/*функция установки флагов и заполнения структур*/
static void set_options(const char option, Options *options) {
  switch (option) {
    case 'e':
      options->e = true;
      patterns_add_from_string(&options->patts, optarg);
      break;
    case 'f':
      options->f = true;
      patterns_add_from_file(&options->patts, optarg);
      break;
    case 'i':
      options->i = true;
      break;
    case 's':
      options->s = true;
      break;
    case 'v':
      options->v = true;
      break;
    case 'n':
      options->n = true;
      break;
    case 'h':
      options->h = true;
      break;
    case 'o':
      options->o = true;
      break;
    case 'l':
      options->l = true;
      break;
    case 'c':
      options->c = true;
      break;
    default:
      printf("Error\n");
      exit(1);
  }
}
/*функция комплимирования шаблонов в регулярные выражения*/
static void patterns_compile_to_regex(Options *const opts) {
  Patterns *patts = &opts->patts;
  patts->reg_data = safe_malloc(sizeof(regex_t) * patts->cur_size);
  int reg_icase = opts->i ? REG_ICASE : 0;
  for (size_t i = 0; i < patts->cur_size; ++i) {
    regcomp(&patts->reg_data[i], patts->data[i], reg_icase);
  }
}
/*Освобождение структуры patts*/
static void patterns_free(Patterns *const patts) {
  for (size_t i = 0; i < patts->cur_size; i++) {
    free(patts->data[i]);
    regfree(&patts->reg_data[i]);
  }
  free(patts->data);
  free(patts->reg_data);
}
/*освобождение всех структур*/
static void options_free(Options *const opts) { patterns_free(&opts->patts); }
/*иницилизация структуры*/
static void patterns_init(Patterns *const patts) {
  patts->cur_size = 0;
  patts->max_size = PATTERNS_INIT;
  patts->data = safe_malloc(sizeof(char *) * patts->max_size);
}
/*проверка соответствия строки шаблону*/
static bool is_match(const char *line, const Options *const opts,
                     regmatch_t *const match) {
  const Patterns *const patts = &opts->patts;
  bool result = false;
  size_t nmatch = match ? 1 : 0;
  for (size_t i = 0; i < patts->cur_size; ++i) {
    if (regexec(&patts->reg_data[i], line, nmatch, match, 0) == 0) {
      result = true;
    }
  }
  if (opts->v) {
    result = !result;
    if (opts->o) {
      result = false;
    }
  }
  return result;
}
/*функция вывода количества совпадений или одного совпадения с выводом*/
static void grep_match_count(FILE *file, const char *filename,
                             const Options *const opts) {
  size_t match_count = 0;
  char *buffer = safe_malloc(sizeof(char) * BUFFER_INIT);
  size_t buffer_size = BUFFER_INIT;
  while (getline(&buffer, &buffer_size, file) != EOF) {
    if (is_match(buffer, opts, NULL)) {
      ++match_count;
    }
  }
  if (opts->file_count > 1 && !opts->h) {
    fprintf(stdout, "%s:", filename);
  }
  fprintf(stdout, "%zu\n", match_count);
  free(buffer);
}

static void grep_lines_with_matches(FILE *file, const char *filename,
                                    const Options *const opts) {
  char *line = safe_malloc(sizeof(char) * BUFFER_INIT);
  size_t line_size = BUFFER_INIT;
  size_t line_count = 0;
  while (getline(&line, &line_size, file) != EOF) {
    ++line_count;
    if (is_match(line, opts, NULL)) {
      if (opts->file_count > 1 && !opts->h) {
        fprintf(stdout, "%s:", filename);
      }
      if (opts->n) {
        fprintf(stdout, "%zu:", line_count);
      }
      fprintf(stdout, "%s", line);
    }
  }
  free(line);
}
/*выводит имена файлов с найдеными совпадениями работает с флагами -h и -n*/
static void grep_only_matching(FILE *file, const char *filename,
                               const Options *const opts) {
  char *line = safe_malloc(sizeof(char) * BUFFER_INIT);
  size_t line_size = BUFFER_INIT;
  size_t line_count = 0;
  regmatch_t match = {0};
  while (getline(&line, &line_size, file) != EOF) {
    char *line_ptr = line;
    ++line_count;
    while (is_match(line_ptr, opts, &match)) {
      if (opts->file_count > 1 && !opts->h) {
        fprintf(stdout, "%s:", filename);
      }
      if (opts->n) {
        fprintf(stdout, "%zu:", line_count);
      }
      fprintf(stdout, "%.*s\n", (int)(match.rm_eo - match.rm_so),
              (line_ptr + (int)match.rm_so));
      line_ptr += (int)match.rm_eo;
    }
  }
  free(line);
}
/*выводит имена файлов, в которых найдено совпадение с заданным шаблоном
 * регулярного выражения*/
static void grep_files_with_matches(FILE *file, const char *filename,
                                    const Options *const opts) {
  char *buffer = safe_malloc(sizeof(char) * BUFFER_INIT);
  size_t buffer_size = BUFFER_INIT;
  while (getline(&buffer, &buffer_size, file) != EOF) {
    if (is_match(buffer, opts, NULL)) {
      fprintf(stdout, "%s\n", filename);
      break;
    }
  }
  free(buffer);
}
/*разбор командной строки и установка флагов*/
void get_options(int argc, char *argv[], Options *opts) {
  patterns_init(&opts->patts);
  int long_options_index = 0;
  char opt =
      getopt_long(argc, argv, SHORTOPTIONS, LONGOPTIONS, &long_options_index);
  while (opt != OPTIONS_END) {
    set_options(opt, opts);
    opt =
        getopt_long(argc, argv, SHORTOPTIONS, LONGOPTIONS, &long_options_index);
  }
  if (!opts->e && !opts->f) {
    patterns_add_from_string(&opts->patts, argv[optind++]);
  }
  patterns_compile_to_regex(opts);
  opts->file_count = argc - optind;
}
static void route_file_greping(FILE *file, const char *filename,
                               const Options *const opts) {
  if (opts->l) {
    grep_files_with_matches(file, filename, opts);
  } else if (opts->c) {
    grep_match_count(file, filename, opts);
  } else if (opts->o) {
    grep_only_matching(file, filename, opts);
  } else {
    grep_lines_with_matches(file, filename, opts);
  }
}

/*опция s*/
static void process_file(int file_count, char *const file_path[],
                         const Options *const opts) {
  for (FILE *curr_file = NULL; file_count--; ++file_path) {
    curr_file = fopen(*file_path, FOPEN_READ);
    if (curr_file != NULL) {
      route_file_greping(curr_file, *file_path, opts);
      fflush(stdout);
      fclose(curr_file);
    } else if (!opts->s) {
      fprintf(stderr, "%s: no file directory\n", *file_path);
    }
  }
}
int main(int argc, char *argv[]) {
  Options opts = {0};
  get_options(argc, argv, &opts);
  process_file(argc - optind, argv + optind, &opts);
  options_free(&opts);
  return 0;
}
