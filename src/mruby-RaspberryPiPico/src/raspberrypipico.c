// SPDX-FileCopyrightText: Copyright (c) 2021 k-mana
// SPDX-License-Identifier: MIT
#include "mruby.h"
#include "mruby/value.h"
#include "mruby/variable.h"

#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/spi.h"

static mrb_value
mrb_time_msleep(mrb_state *mrb, mrb_value self)
{
  mrb_int msec;
  mrb_get_args(mrb, "i", &msec);
  sleep_ms(msec);
  return mrb_nil_value();
}

// static mrb_value
// mrb_time_usleep(mrb_state *mrb, mrb_value self)
// {
//   mrb_int usec;
//   mrb_get_args(mrb, "i", &usec);
//   sleep_us(usec);
//   return mrb_nil_value();
// }

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

static mrb_value
mrb_kernel_sleep(mrb_state *mrb, mrb_value self)
{
  mrb_float sec;
  mrb_get_args(mrb, "f", &sec);
  sleep_ms((mrb_int)(sec * 1000.0));
  return mrb_nil_value();
}

#define ENABLE_MRUBY_SPI
#ifdef ENABLE_MRUBY_SPI
#define PIN_auto_cs0   17
#define PIN_SCK0  18
#define PIN_MOSI0 19
#define PIN_MISO0 16

static spi_inst_t *_spi = spi0;

static mrb_value
mrb_spi_init(mrb_state *mrb, mrb_value self)
{
  // Initialize SPI
#if 0
  // auto_cs pin
  gpio_init(PIN_auto_cs0);
  gpio_set_dir(PIN_auto_cs0, GPIO_OUT);
  gpio_put(PIN_auto_cs0, 1);

  // SPI port at 1MHz
  spi_init(_spi, 1000 * 1000);

  // SPI format
  spi_set_format(
    _spi,
    8,    // Number of bits
    1,    // Polarity(CPOL)
    1,    // Phase(CPHA)
    SPI_MSB_FIRST
  );

  // Initialize SPI pins
  gpio_set_function(PIN_SCK0, GPIO_FUNC_SPI);
  gpio_set_function(PIN_MOSI0, GPIO_FUNC_SPI);
  gpio_set_function(PIN_MISO0, GPIO_FUNC_SPI);
#else
  spi_init(_spi, 1000*1000);
#endif

  return self;
}

static mrb_value
mrb_spi_write(mrb_state *mrb, mrb_value self)
{
  mrb_int reg, data;
  mrb_bool auto_cs = true;
  uint8_t msg[2] = {0, 0};
  mrb_get_args(mrb, "ii|b", &reg, &data, &auto_cs);

  msg[0] = (uint8_t)reg;
  msg[1] = (uint8_t)data;

  if (auto_cs) gpio_put(PIN_auto_cs0, 0);
  spi_write_blocking(_spi, msg, 2);
  if (auto_cs) gpio_put(PIN_auto_cs0, 1);

  return self;
}

static mrb_value
mrb_spi_read(mrb_state *mrb, mrb_value self)
{
  mrb_int reg, len=1;
  mrb_bool auto_cs = true;
  mrb_get_args(mrb, "i|ib", &reg, &len, &auto_cs);

  uint8_t mb = (len == 1) ? 0 : 1;
  uint8_t msg = 0x80 | (mb << 6) | (uint8_t)reg;
  uint8_t *buf = (uint8_t*)mrb_malloc(mrb, len);

  if (auto_cs) gpio_put(PIN_auto_cs0, 0);
  spi_write_blocking(_spi, &msg, 1);
  int num_bytes = spi_read_blocking(_spi, 0, buf, len);
  if (auto_cs) gpio_put(PIN_auto_cs0, 1);

  mrb_value val = mrb_str_new(mrb, buf, num_bytes);
  mrb_free(mrb, buf);

  return val;
}

static mrb_value
mrb_spi_start(mrb_state *mrb, mrb_value self)
{
  gpio_put(PIN_auto_cs0, 0);
  return self;
}

static mrb_value
mrb_spi_end(mrb_state *mrb, mrb_value self)
{
  gpio_put(PIN_auto_cs0, 1);
  return self;
}
#endif

void dump_memory(uint8_t *addr, uint32_t length)
{
  uint32_t ofst = 0;
  while (ofst < length) {
    if (ofst % 256 == 0) puts("address : +0 +1 +2 +3 +4 +5 +6 +7 +8 +9 +A +B +C +D +E +F");
    printf("%08lX:", (uint32_t)addr + ofst);
    uint32_t len = 16;
    if (ofst + len > length) len = length - ofst;
    for (uint32_t i=0; i<len; i++) {
      printf(" %02X", addr[ofst + i]);
    }
    puts("");
    ofst += 16;
  }
}
//
// memdump(addr, len=0x100) => nil
// args:
//  addr: Memory address to dump
//  len:  Memory length to dump
static mrb_value
mrb_memory_dump(mrb_state *mrb, mrb_value self)
{
  mrb_int addr, len=0x100;
  mrb_get_args(mrb, "i|i", &addr, &len);
  dump_memory((uint8_t*)addr, (uint32_t)len);
  return mrb_nil_value();
}

void
mrb_mruby_raspberrypipico_gem_init(mrb_state *mrb)
{
  // struct RClass *rasp, *time;
  // rasp = mrb_define_module(mrb, "RaspberryPiPico");
  // time = mrb_define_module_under(mrb, rasp, "Time");
  // mrb_define_class_method(mrb, time, "msleep", mrb_time_msleep, MRB_ARGS_REQ(1));
  // mrb_define_class_method(mrb, time, "usleep", mrb_time_usleep, MRB_ARGS_REQ(1));

  // Kernel
  mrb_define_method(mrb, mrb->kernel_module, "sleep", mrb_kernel_sleep, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, mrb->kernel_module, "msleep", mrb_time_msleep, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, mrb->kernel_module, "memdump", mrb_memory_dump, MRB_ARGS_ARG(1, 1));

  // GPIO
  struct RClass *gpio = mrb_define_class(mrb, "GPIO", mrb->object_class);
  mrb_const_set(mrb, mrb_obj_value(gpio), mrb_intern_lit(mrb, "OUT"), mrb_fixnum_value(GPIO_OUT));
  mrb_const_set(mrb, mrb_obj_value(gpio), mrb_intern_lit(mrb, "IN"), mrb_fixnum_value(GPIO_IN));
  mrb_define_method(mrb, gpio, "initialize", mrb_gpio_init, MRB_ARGS_ARG(1, 1));
  mrb_define_method(mrb, gpio, "setmode", mrb_gpio_setmode, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, gpio, "write", mrb_gpio_write, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, gpio, "read", mrb_gpio_read, MRB_ARGS_NONE());

#ifdef ENABLE_MRUBY_SPI
  // SPI
  struct RClass *spi = mrb_define_class(mrb, "SPI", mrb->object_class);
  mrb_define_method(mrb, spi, "initialize", mrb_spi_init, MRB_ARGS_NONE());
  mrb_define_method(mrb, spi, "write", mrb_spi_write, MRB_ARGS_ARG(2, 1));
  mrb_define_method(mrb, spi, "read", mrb_spi_read, MRB_ARGS_ARG(1, 2));
  mrb_define_method(mrb, spi, "_start", mrb_spi_start, MRB_ARGS_NONE());
  mrb_define_method(mrb, spi, "_end", mrb_spi_end, MRB_ARGS_NONE());
#endif
}

void
mrb_mruby_raspberrypipico_gem_final(mrb_state *mrb)
{
}
