
#undef NDEBUG  /* ensure tests always assert. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "upb_decoder.c"
#include "upb_def.h"
#include "upb_glue.h"

int num_assertions = 0;
#define ASSERT(expr) do { \
  ++num_assertions; \
  assert(expr); \
  } while(0)

static void test_get_v_uint64_t()
{
#define TEST(name, bytes, val) {\
    upb_status status = UPB_STATUS_INIT; \
    const char name[] = bytes "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff" ; \
    const char *name ## _buf = name; \
    uint64_t name ## _val = 0; \
    upb_decode_varint_fast(&name ## _buf, &name ## _val, &status); \
    ASSERT(upb_ok(&status)); \
    ASSERT(name ## _val == val); \
    ASSERT(name ## _buf == name + sizeof(name) - 16);  /* - 1 for NULL */ \
  }

  TEST(zero,   "\x00",                                                      0ULL);
  TEST(one,    "\x01",                                                      1ULL);
  TEST(twob,   "\x81\x14",                                              0xa01ULL);
  TEST(twob,   "\x81\x03",                                              0x181ULL);
  TEST(threeb, "\x81\x83\x07",                                        0x1c181ULL);
  TEST(fourb,  "\x81\x83\x87\x0f",                                  0x1e1c181ULL);
  TEST(fiveb,  "\x81\x83\x87\x8f\x1f",                            0x1f1e1c181ULL);
  TEST(sixb,   "\x81\x83\x87\x8f\x9f\x3f",                      0x1f9f1e1c181ULL);
  TEST(sevenb, "\x81\x83\x87\x8f\x9f\xbf\x7f",                0x1fdf9f1e1c181ULL);
  TEST(eightb, "\x81\x83\x87\x8f\x9f\xbf\xff\x01",            0x3fdf9f1e1c181ULL);
  TEST(nineb,  "\x81\x83\x87\x8f\x9f\xbf\xff\x81\x03",      0x303fdf9f1e1c181ULL);
  TEST(tenb,   "\x81\x83\x87\x8f\x9f\xbf\xff\x81\x83\x07", 0x8303fdf9f1e1c181ULL);
#undef TEST

  char twelvebyte[16] = {0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01, 0x01};
  const char *twelvebyte_buf = twelvebyte;
  uint64_t twelvebyte_val = 0;
  upb_status status = UPB_STATUS_INIT;
  /* A varint that terminates before hitting the end of the provided buffer,
   * but in too many bytes (11 instead of 10). */
  upb_decode_varint_fast(&twelvebyte_buf, &twelvebyte_val, &status);
  ASSERT(status.code == UPB_ERROR);
  upb_status_uninit(&status);
}

#if 0
static void test_get_v_uint32_t()
{
#define TEST(name, bytes, val) {\
    upb_status status = UPB_STATUS_INIT; \
    const uint8_t name[] = bytes; \
    const uint8_t *name ## _buf = name; \
    uint32_t name ## _val = 0; \
    name ## _buf = upb_get_v_uint32_t(name, name + sizeof(name), &name ## _val, &status); \
    ASSERT(upb_ok(&status)); \
    ASSERT(name ## _val == val); \
    ASSERT(name ## _buf == name + sizeof(name) - 1);  /* - 1 for NULL */ \
    /* Test NEED_MORE_DATA. */ \
    if(sizeof(name) > 2) { \
      name ## _buf = upb_get_v_uint32_t(name, name + sizeof(name) - 2, &name ## _val, &status); \
      ASSERT(status.code == UPB_STATUS_NEED_MORE_DATA); \
    } \
  }

  TEST(zero,   "\x00",                                              0UL);
  TEST(one,    "\x01",                                              1UL);
  TEST(twob,   "\x81\x03",                                      0x181UL);
  TEST(threeb, "\x81\x83\x07",                                0x1c181UL);
  TEST(fourb,  "\x81\x83\x87\x0f",                          0x1e1c181UL);
  /* get_v_uint32_t truncates, so all the rest return the same thing. */
  TEST(fiveb,  "\x81\x83\x87\x8f\x1f",                     0xf1e1c181UL);
  TEST(sixb,   "\x81\x83\x87\x8f\x9f\x3f",                 0xf1e1c181UL);
  TEST(sevenb, "\x81\x83\x87\x8f\x9f\xbf\x7f",             0xf1e1c181UL);
  TEST(eightb, "\x81\x83\x87\x8f\x9f\xbf\xff\x01",         0xf1e1c181UL);
  TEST(nineb,  "\x81\x83\x87\x8f\x9f\xbf\xff\x81\x03",     0xf1e1c181UL);
  TEST(tenb,   "\x81\x83\x87\x8f\x9f\xbf\xff\x81\x83\x07", 0xf1e1c181UL);
#undef TEST

  uint8_t twelvebyte[] = {0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01, 0x01};
  uint32_t twelvebyte_val = 0;
  upb_status status = UPB_STATUS_INIT;
  /* A varint that terminates before hitting the end of the provided buffer,
   * but in too many bytes (11 instead of 10). */
  upb_get_v_uint32_t(twelvebyte, twelvebyte + 12, &twelvebyte_val, &status);
  ASSERT(status.code == UPB_ERROR_UNTERMINATED_VARINT);

  /* A varint that terminates simultaneously with the end of the provided
   * buffer, but in too many bytes (11 instead of 10). */
  upb_reset(&status);
  upb_get_v_uint32_t(twelvebyte, twelvebyte + 11, &twelvebyte_val, &status);
  ASSERT(status.code == UPB_ERROR_UNTERMINATED_VARINT);

  /* A varint whose buffer ends on exactly the byte where the varint must
   * terminate, but the final byte does not terminate.  The absolutely most
   * correct return code here is UPB_ERROR_UNTERMINATED_VARINT, because we know
   * by this point that the varint does not properly terminate.  But we also
   * allow a return value of UPB_STATUS_NEED_MORE_DATA here, because it does not
   * compromise overall correctness -- clients who supply more data later will
   * then receive a UPB_ERROR_UNTERMINATED_VARINT error; clients who have no
   * more data to supply will (rightly) conclude that their protobuf is corrupt.
   */
  upb_reset(&status);
  upb_get_v_uint32_t(twelvebyte, twelvebyte + 10, &twelvebyte_val, &status);
  ASSERT(status.code == UPB_ERROR_UNTERMINATED_VARINT ||
         status.code == UPB_STATUS_NEED_MORE_DATA);

  upb_reset(&status);
  upb_get_v_uint32_t(twelvebyte, twelvebyte + 9, &twelvebyte_val, &status);
  ASSERT(status.code == UPB_STATUS_NEED_MORE_DATA);
}

static void test_skip_v_uint64_t()
{
#define TEST(name, bytes) {\
    upb_status status = UPB_STATUS_INIT; \
    const uint8_t name[] = bytes; \
    const uint8_t *name ## _buf = name; \
    name ## _buf = upb_skip_v_uint64_t(name ## _buf, name + sizeof(name), &status); \
    ASSERT(upb_ok(&status)); \
    ASSERT(name ## _buf == name + sizeof(name) - 1);  /* - 1 for NULL */ \
    /* Test NEED_MORE_DATA. */ \
    if(sizeof(name) > 2) { \
      name ## _buf = upb_skip_v_uint64_t(name, name + sizeof(name) - 2, &status); \
      ASSERT(status.code == UPB_STATUS_NEED_MORE_DATA); \
    } \
  }

  TEST(zero,   "\x00");
  TEST(one,    "\x01");
  TEST(twob,   "\x81\x03");
  TEST(threeb, "\x81\x83\x07");
  TEST(fourb,  "\x81\x83\x87\x0f");
  TEST(fiveb,  "\x81\x83\x87\x8f\x1f");
  TEST(sixb,   "\x81\x83\x87\x8f\x9f\x3f");
  TEST(sevenb, "\x81\x83\x87\x8f\x9f\xbf\x7f");
  TEST(eightb, "\x81\x83\x87\x8f\x9f\xbf\xff\x01");
  TEST(nineb,  "\x81\x83\x87\x8f\x9f\xbf\xff\x81\x03");
  TEST(tenb,   "\x81\x83\x87\x8f\x9f\xbf\xff\x81\x83\x07");
#undef TEST

  uint8_t twelvebyte[] = {0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01, 0x01};
  upb_status status = UPB_STATUS_INIT;
  /* A varint that terminates before hitting the end of the provided buffer,
   * but in too many bytes (11 instead of 10). */
  upb_skip_v_uint64_t(twelvebyte, twelvebyte + 12, &status);
  ASSERT(status.code == UPB_ERROR_UNTERMINATED_VARINT);

  /* A varint that terminates simultaneously with the end of the provided
   * buffer, but in too many bytes (11 instead of 10). */
  upb_reset(&status);
  upb_skip_v_uint64_t(twelvebyte, twelvebyte + 11, &status);
  ASSERT(status.code == UPB_ERROR_UNTERMINATED_VARINT);

  /* A varint whose buffer ends on exactly the byte where the varint must
   * terminate, but the final byte does not terminate.  The absolutely most
   * correct return code here is UPB_ERROR_UNTERMINATED_VARINT, because we know
   * by this point that the varint does not properly terminate.  But we also
   * allow a return value of UPB_STATUS_NEED_MORE_DATA here, because it does not
   * compromise overall correctness -- clients who supply more data later will
   * then receive a UPB_ERROR_UNTERMINATED_VARINT error; clients who have no
   * more data to supply will (rightly) conclude that their protobuf is corrupt.
   */
  upb_reset(&status);
  upb_skip_v_uint64_t(twelvebyte, twelvebyte + 10, &status);
  ASSERT(status.code == UPB_ERROR_UNTERMINATED_VARINT ||
         status.code == UPB_STATUS_NEED_MORE_DATA);

  upb_reset(&status);
  upb_skip_v_uint64_t(twelvebyte, twelvebyte + 9, &status);
  ASSERT(status.code == UPB_STATUS_NEED_MORE_DATA);
}

static void test_get_f_uint32_t()
{
#define TEST(name, bytes, val) {\
    upb_status status = UPB_STATUS_INIT; \
    const uint8_t name[] = bytes; \
    const uint8_t *name ## _buf = name; \
    uint32_t name ## _val = 0; \
    name ## _buf = upb_get_f_uint32_t(name ## _buf, name + sizeof(name), &name ## _val, &status); \
    ASSERT(upb_ok(&status)); \
    ASSERT(name ## _val == val); \
    ASSERT(name ## _buf == name + sizeof(name) - 1);  /* - 1 for NULL */ \
  }

  TEST(zero,  "\x00\x00\x00\x00",                                0x0UL);
  TEST(one,   "\x01\x00\x00\x00",                                0x1UL);

  uint8_t threeb[] = {0x00, 0x00, 0x00};
  uint32_t threeb_val;
  upb_status status = UPB_STATUS_INIT;
  upb_get_f_uint32_t(threeb, threeb + sizeof(threeb), &threeb_val, &status);
  ASSERT(status.code == UPB_STATUS_NEED_MORE_DATA);

#undef TEST
}
#endif

static void test_upb_symtab() {
  upb_symtab *s = upb_symtab_new();
  upb_symtab_add_descriptorproto(s);
  ASSERT(s);
  upb_string *descriptor = upb_strreadfile("tests/test.proto.pb");
  if(!descriptor) {
    fprintf(stderr, "Couldn't read input file tests/test.proto.pb\n");
    exit(1);
  }
  upb_status status = UPB_STATUS_INIT;
  upb_parsedesc(s, descriptor, &status);
  upb_printerr(&status);
  ASSERT(upb_ok(&status));
  upb_status_uninit(&status);
  upb_string_unref(descriptor);

  // Test cycle detection by making a cyclic def's main refcount go to zero
  // and then be incremented to one again.
  upb_string *symname = upb_strdupc("A");
  upb_def *def = upb_symtab_lookup(s, symname);
  upb_string_unref(symname);
  ASSERT(def);
  upb_symtab_unref(s);
  upb_msgdef *m = upb_downcast_msgdef(def);
  upb_msg_iter i = upb_msg_begin(m);
  upb_fielddef *f = upb_msg_iter_field(i);
  ASSERT(upb_hasdef(f));
  upb_def *def2 = f->def;

  i = upb_msg_next(m, i);
  ASSERT(upb_msg_done(i));  // "A" should only have one field.

  ASSERT(upb_downcast_msgdef(def2));
  upb_def_ref(def2);
  upb_def_unref(def);
  upb_def_unref(def2);


}


int main()
{
#define TEST(func) do { \
  int assertions_before = num_assertions; \
  printf("Running " #func "..."); fflush(stdout); \
  func(); \
  printf("ok (%d assertions).\n", num_assertions - assertions_before); \
  } while (0)

  TEST(test_get_v_uint64_t);
  //TEST(test_get_v_uint32_t);
  //TEST(test_skip_v_uint64_t);
  //TEST(test_get_f_uint32_t);
  TEST(test_upb_symtab);
  printf("All tests passed (%d assertions).\n", num_assertions);
  return 0;
}
