// SPDX-FileCopyrightText: Copyright (c) 2021 k-mana
// SPDX-License-Identifier: MIT
#include "mruby.h"
#include "mruby/value.h"
#include "mruby/variable.h"

#include "pico/stdlib.h"
#include "pico/time.h"

static mrb_value
mrb_time_msleep(mrb_state *mrb, mrb_value self)
{
  mrb_int msec;
  mrb_get_args(mrb, "i", &msec);
  sleep_ms(msec);
  return mrb_nil_value();
}

static mrb_value
mrb_time_usleep(mrb_state *mrb, mrb_value self)
{
  mrb_int usec;
  mrb_get_args(mrb, "i", &usec);
  sleep_us(usec);
  return mrb_nil_value();
}

static mrb_value
mrb_gpio_init(mrb_state *mrb, mrb_value self)
{
  mrb_int pin=0, mode=GPIO_OUT;
  mrb_get_args(mrb, "i|i", &pin, &mode);
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "@pin"), mrb_fixnum_value(pin));
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "@mode"), mrb_fixnum_value(mode));
  gpio_init(pin);
  gpio_set_dir(pin, mode);
  return self;
}
static mrb_value
mrb_gpio_setmode(mrb_state *mrb, mrb_value self)
{
  mrb_int mode;
  mrb_value opin = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@pin"));
  mrb_get_args(mrb, "i", &mode);
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "@mode"), mrb_fixnum_value(mode));
  if (mode != GPIO_OUT && mode != GPIO_IN) {
    // TODO: mode error
    //
  }
  gpio_set_dir(mrb_integer(opin), mode);
  return mrb_nil_value();
}
static mrb_value
mrb_gpio_write(mrb_state *mrb, mrb_value self)
{
  mrb_int val;
  mrb_value opin = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@pin"));
  mrb_value omode = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@mode"));
  mrb_get_args(mrb, "i", &val);
  if (mrb_integer(omode) != GPIO_OUT) {
    // TODO: mode error
    //
    return mrb_nil_value();
  }
  // write value to GPIO
  gpio_put(mrb_integer(opin),val);
  return mrb_nil_value();
}
static mrb_value
mrb_gpio_read(mrb_state *mrb, mrb_value self)
{
  mrb_int val = 0;
  mrb_value opin, omode;
  opin = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@pin"));
  omode = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@mode"));
  if (mrb_integer(omode) != GPIO_IN) {
    // TODO: mode error
    //
    return mrb_fixnum_value(val);
  }
  // read value from GPIO
  val = gpio_get(mrb_integer(opin));
  return mrb_fixnum_value(val);
}

const uint8_t hoge[] = {
  0
};

void
mrb_mruby_raspberrypipico_gem_init(mrb_state *mrb)
{
  struct RClass *rasp, *time;
  rasp = mrb_define_module(mrb, "RaspberryPiPico");
  time = mrb_define_module_under(mrb, rasp, "Time");
  mrb_define_class_method(mrb, time, "msleep", mrb_time_msleep, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, time, "usleep", mrb_time_usleep, MRB_ARGS_REQ(1));

  {
    struct RClass *gpio = mrb_define_class(mrb, "GPIO", mrb->object_class);
    mrb_const_set(mrb, mrb_obj_value(gpio), mrb_intern_lit(mrb, "OUT"), mrb_fixnum_value(GPIO_OUT));
    mrb_const_set(mrb, mrb_obj_value(gpio), mrb_intern_lit(mrb, "IN"), mrb_fixnum_value(GPIO_IN));
    mrb_define_method(mrb, gpio, "initialize", mrb_gpio_init, MRB_ARGS_ARG(1, 1));
    mrb_define_method(mrb, gpio, "setmode", mrb_gpio_setmode, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, gpio, "write", mrb_gpio_write, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, gpio, "read", mrb_gpio_read, MRB_ARGS_NONE());
    // for test
    printf("hoge addr: %p\n", hoge);
    uint8_t *mrbarea = (uint8_t*)0x10100000;
    printf("mrbarea: %x %x %x %x\n", mrbarea[0], mrbarea[1], mrbarea[2], mrbarea[3]);
  }
}

void
mrb_mruby_raspberrypipico_gem_final(mrb_state *mrb)
{
}
