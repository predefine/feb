#include <fe.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/stat.h>

enum feb_target {
  FEB_TARGET_CC,
  FEB_TARGET_LD,
};

typedef struct target {
  enum feb_target type;
  bool have_extras;
  fe_Object *inputs;
  fe_Object *outputs;
  fe_Object *extra;
} target_t;

#define fe_list_foreach(ctx, list, var, tmpvar)                                \
  for (fe_Object *var = fe_car(ctx, list), *tmpvar = fe_cdr(ctx, list);        \
       var != NULL; var = (tmpvar != NULL && fe_type(ctx, tmpvar) == FE_TPAIR) \
                              ? fe_car(ctx, tmpvar)                            \
                              : NULL,                                          \
                 tmpvar = var != NULL ? fe_cdr(ctx, tmpvar) : NULL)

#define as_list(ctx, var)                                                      \
  if (fe_type(ctx, var) != FE_TPAIR)                                           \
    var = fe_cons(ctx, var, fe_bool(ctx, 0));

static unsigned int fe_list_len(fe_Context *ctx, fe_Object *obj) {
  unsigned int len = 0;
  while (!fe_isnil(ctx, obj)) {
    len++;
    obj = fe_cdr(ctx, obj);
  }
  return len;
}

static bool feb_should_rebuild(char *input, char *output) {
  struct stat input_stat, output_stat;
  stat(input, &input_stat);
  if (stat(output, &output_stat) != 0)
    return true;
  return (input_stat.st_mtim.tv_sec > output_stat.st_mtim.tv_sec) ? true
                                                                  : false;
}

static void feb_build_target(fe_Context *ctx, target_t target) {
  switch (target.type) {
  case FEB_TARGET_CC: {
    if (fe_list_len(ctx, target.inputs) != fe_list_len(ctx, target.outputs))
      fe_error(ctx, "cc should output same count of files as input");

    char extra[128] = {0};

    if (target.have_extras) {
      if (fe_list_len(ctx, target.extra) != 1)
        fe_error(ctx,
                 "cc extra should be string or list which contain 1 string");
      fe_tostring(ctx, fe_car(ctx, target.extra), &extra[0], sizeof(extra));
    }

    fe_list_foreach(ctx, target.inputs, input, tmpvar) {
      fe_Object *output = fe_car(ctx, target.outputs);
      target.outputs = fe_cdr(ctx, target.outputs);

      char input_str[128] = {0};
      char output_str[128] = {0};
      fe_tostring(ctx, input, &input_str[0], sizeof(input_str));
      fe_tostring(ctx, output, &output_str[0], sizeof(output_str));

      if (!feb_should_rebuild(input_str, output_str))
        continue;

      char command[512] = {0};

      snprintf(command, sizeof(command), "%s %s -c -o %s %s",
               getenv("FEB_CC") ?: "cc", input_str, output_str, extra);

      printf("CC %s\n", output_str);
      if (system(command) != 0)
        fe_error(ctx, "cc failed");
    }
    break;
  }
  case FEB_TARGET_LD: {
    if (fe_list_len(ctx, target.outputs) > 1)
      fe_error(ctx, "ld cannot output multiply files");

    char extra[128] = {0};

    if (target.have_extras) {
      if (fe_list_len(ctx, target.extra) != 1)
        fe_error(ctx,
                 "ld extra should be string or list which contain 1 string");
      fe_tostring(ctx, fe_car(ctx, target.extra), &extra[0], sizeof(extra));
    }

    char output_str[128] = {0};
    fe_tostring(ctx, fe_car(ctx, target.outputs), &output_str[0],
                sizeof(output_str));

    char inputs[4096] = {0};
    unsigned int input_pos = 0;
    bool should_rebuild = 0;

    fe_list_foreach(ctx, target.inputs, input, tmpvar) {
      unsigned int str_len = fe_tostring(ctx, input, &inputs[input_pos],
                                         sizeof(inputs) - input_pos);
      if (!should_rebuild && feb_should_rebuild(inputs + input_pos, output_str))
        should_rebuild = 1;
      input_pos += str_len + 1;
      inputs[input_pos - 1] = ' ';
      inputs[input_pos] = 0;
    }

    if(!should_rebuild)
      return;

    char command[input_pos + 384];
    snprintf(command, sizeof(command), "%s %s -o %s %s",
             getenv("FEB_LD") ?: "cc", inputs, output_str, extra);

    printf("CCLD %s\n", output_str);
    if (system(command) != 0)
      fe_error(ctx, "ld failed");
    break;
  }
  default:
    fprintf(stderr, "Unknown target with type: %d\n", target.type);
    break;
  }
}

static fe_Object *feb_build(fe_Context *ctx, fe_Object *arg) {
  target_t target = {};

  target.type = fe_tonumber(ctx, fe_nextarg(ctx, &arg));

  fe_Object *inputs = fe_nextarg(ctx, &arg);
  fe_Object *outputs = fe_nextarg(ctx, &arg);
  as_list(ctx, inputs);
  as_list(ctx, outputs);
  target.inputs = inputs;
  target.outputs = outputs;

  if (fe_type(ctx, arg) == FE_TPAIR) {
    fe_Object *extra = fe_nextarg(ctx, &arg);
    as_list(ctx, extra);
    target.have_extras = true;
    target.extra = extra;
  }

  feb_build_target(ctx, target);

  if(fe_list_len(ctx, outputs) == 1)
    return fe_car(ctx, outputs);
  return outputs;
}

int main(int argc, char **argv) {
  int gc;
  fe_Object *obj;
  FILE *volatile fp = stdin;
  char buf[64000];
  fe_Context *ctx = fe_open(buf, sizeof(buf));

  if (argc < 2) {
    fprintf(stderr, "usage: %s <build.fe>\n", argv[0]);
    return EXIT_FAILURE;
  }

  fp = fopen(argv[1], "rb");
  if (!fp) {
    fe_error(ctx, "could not open input file");
  }

  gc = fe_savegc(ctx);

  fe_set(ctx, fe_symbol(ctx, "build"), fe_cfunc(ctx, feb_build));
  fe_set(ctx, fe_symbol(ctx, "cc"), fe_number(ctx, FEB_TARGET_CC));
  fe_set(ctx, fe_symbol(ctx, "ld"), fe_number(ctx, FEB_TARGET_LD));

  for (;;) {
    fe_restoregc(ctx, gc);
    if (!(obj = fe_readfp(ctx, fp))) {
      break;
    }
    obj = fe_eval(ctx, obj);
  }

  return EXIT_SUCCESS;
}

