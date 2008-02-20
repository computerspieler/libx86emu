/****************************************************************************
*
*						Realmode X86 Emulator Library
*
*            	Copyright (C) 1996-1999 SciTech Software, Inc.
* 				     Copyright (C) David Mosberger-Tang
* 					   Copyright (C) 1999 Egbert Eich
*
*  ========================================================================
*
*  Permission to use, copy, modify, distribute, and sell this software and
*  its documentation for any purpose is hereby granted without fee,
*  provided that the above copyright notice appear in all copies and that
*  both that copyright notice and this permission notice appear in
*  supporting documentation, and that the name of the authors not be used
*  in advertising or publicity pertaining to distribution of the software
*  without specific, written prior permission.  The authors makes no
*  representations about the suitability of this software for any purpose.
*  It is provided "as is" without express or implied warranty.
*
*  THE AUTHORS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
*  INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
*  EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
*  CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
*  USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
*  OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
*  PERFORMANCE OF THIS SOFTWARE.
*
*  ========================================================================
*
* Language:		ANSI C
* Environment:	Any
* Developer:    Kendall Bennett
*
* Description:  This file includes subroutines to implement the decoding
*               and emulation of all the x86 extended two-byte processor
*               instructions.
*
****************************************************************************/

#include "include/x86emui.h"

/*----------------------------- Implementation ----------------------------*/


/****************************************************************************
PARAMETERS:
op2 - Instruction op code

REMARKS:
Handles illegal opcodes.
****************************************************************************/
static void x86emuOp2_illegal_op(u8 op2)
{
  OP_DECODE("illegal opcode");

  INTR_RAISE_UD;
}


/****************************************************************************
REMARKS:
Handles opcode 0x0f,0x00
****************************************************************************/
static void x86emuOp2_opc_00(u8 X86EMU_UNUSED(op2))
{
  int mod, rl, rh;
  u16 *reg16;
  u32 addr, val;

  fetch_decode_modrm(&mod, &rh, &rl);

  switch(rh) {
    case 0:
      OP_DECODE("SLDT");
      break;
    case 1:
      OP_DECODE("STR");
      break;
    case 2:
      OP_DECODE("LLDT");
      break;
    case 3:
      OP_DECODE("LTR");
      break;
    case 4:
      OP_DECODE("VERR");
      break;
    case 5:
      OP_DECODE("VERW");
      break;
    default:
      INTR_RAISE_UD;
      return;
  }

  if(mod == 3) {
    reg16 = decode_rm_word_register(rl);
    switch(rh) {
      case 0:	/* SLDT */
        *reg16 = M.x86.R_LDT;
        break;

      case 1:	/* STR */
        *reg16 = M.x86.R_TR;
        break;

      case 2:	/* LLDT */
        decode_set_seg_register(&M.x86.ldt, *reg16);
        break;

      case 3:	/* LTR */
        M.x86.R_TR = *reg16;
        break;

      case 4:	/* VERR */
        if(*reg16 != 0) SET_FLAG(F_ZF);
        break;

      case 5:	/* VERW*/
        if(*reg16 != 0) SET_FLAG(F_ZF);
        break;
    }
  }
  else {
    addr = decode_rm_address(mod, rl);
    switch(rh) {
      case 0:	/* SLDT */
        store_data_word(addr, M.x86.R_LDT);
        break;

      case 1:	/* STR */
        store_data_word(addr, M.x86.R_TR);
        break;

      case 2:	/* LLDT */
        val = fetch_data_word(addr);
        decode_set_seg_register(&M.x86.ldt, val);
        break;

      case 3:	/* LTR */
        val = fetch_data_word(addr);
        M.x86.R_TR = val;
        break;

      case 4:	/* VERR */
        val = fetch_data_word(addr);
        if(val != 0) SET_FLAG(F_ZF);
        break;

      case 5:	/* VERW*/
        val = fetch_data_word(addr);
        if(val != 0) SET_FLAG(F_ZF);
        break;
    }
  }
}


/****************************************************************************
REMARKS:
Handles opcode 0x0f,0x01
****************************************************************************/
static void x86emuOp2_opc_01(u8 X86EMU_UNUSED(op2))
{
  int mod, rl, rh;
  u16 *reg16;
  u32 base, addr, val;
  u16 limit;

  fetch_decode_modrm(&mod, &rh, &rl);

  if(mod == 3 && rh != 5 && rh != 6) {
    INTR_RAISE_UD;
  }
  else {
    switch(rh) {
      case 0:	/* sgdt */
        OP_DECODE("sgdt ");
        addr = decode_rm_address(mod, rl);
        base = M.x86.gdt.base;
        if(!MODE_DATA32) base &= 0xffffff;
        store_data_word(addr, M.x86.gdt.limit);
        store_data_long(addr + 2, base);
        break;

      case 1:	/* sidt */
        OP_DECODE("sidt ");
        addr = decode_rm_address(mod, rl);
        base = M.x86.idt.base;
        if(!MODE_DATA32) base &= 0xffffff;
        store_data_word(addr, M.x86.idt.limit);
        store_data_long(addr + 2, base);
        break;

      case 2:	/* lgdt */
        OP_DECODE("lgdt ");
        addr = decode_rm_address(mod, rl);
        limit = fetch_data_word(addr);
        base = fetch_data_long(addr + 2);
        if(!MODE_DATA32) base &= 0xffffff;
        M.x86.gdt.limit = limit;
        M.x86.gdt.base = base;
        break;

      case 3:	/* lidt */
        OP_DECODE("lidt ");
        addr = decode_rm_address(mod, rl);
        limit = fetch_data_word(addr);
        base = fetch_data_long(addr + 2);
        if(!MODE_DATA32) base &= 0xffffff;
        M.x86.idt.limit = limit;
        M.x86.idt.base = base;
        break;

      case 4:
        INTR_RAISE_UD;
        break;

      case 5:
        OP_DECODE("smsw ");
        if(mod == 3) {
          reg16 = decode_rm_word_register(rl);
          *reg16 = M.x86.R_CR0;
        }
        else {
          addr = decode_rm_address(mod, rl);
          store_data_word(addr, M.x86.R_CR0);
        }
        break;

      case 6:
        OP_DECODE("lmsw ");
        if(mod == 3) {
          reg16 = decode_rm_word_register(rl);
          M.x86.R_CR0 = (M.x86.R_CR0 & ~0xffff) + *reg16;
        }
        else {
          addr = decode_rm_address(mod, rl);
          val = fetch_data_word(addr);
          M.x86.R_CR0 = (M.x86.R_CR0 & ~0xffff) + val;
        }
        break;

      case 7:
        OP_DECODE("invlpg ");
        decode_rm_address(mod, rl);
        break;
    }
  }
}


/****************************************************************************
REMARKS:
Handles opcode 0x0f,0x20
****************************************************************************/
static void x86emuOp_mov_word_RM_CRx(u8 X86EMU_UNUSED(op2))
{
  int mod, rl, rh;
  u32 *reg32;

  OP_DECODE("mov ");
  fetch_decode_modrm(&mod, &rh, &rl);

  if(mod == 3) {
    reg32 = decode_rm_long_register(rl);
    OP_DECODE(",cr");
    DECODE_HEX1(rh);
    *reg32 = M.x86.crx[rh];
  }
  else {
    INTR_RAISE_UD;
  }
}


/****************************************************************************
REMARKS:
Handles opcode 0x0f,0x21
****************************************************************************/
static void x86emuOp_mov_word_RM_DRx(u8 X86EMU_UNUSED(op2))
{
  int mod, rl, rh;
  u32 *reg32;

  OP_DECODE("mov ");
  fetch_decode_modrm(&mod, &rh, &rl);

  if(mod == 3) {
    reg32 = decode_rm_long_register(rl);
    OP_DECODE(",dr");
    DECODE_HEX1(rh);
    *reg32 = M.x86.drx[rh];
  }
  else {
    INTR_RAISE_UD;
  }
}


/****************************************************************************
REMARKS:
Handles opcode 0x0f,0x22
****************************************************************************/
static void x86emuOp_mov_word_CRx_RM(u8 X86EMU_UNUSED(op2))
{
  int mod, rl, rh;

  OP_DECODE("mov cr");
  fetch_decode_modrm(&mod, &rh, &rl);
  DECODE_HEX1(rh);
  OP_DECODE(",");

  if(mod == 3) {
    M.x86.crx[rh] = *decode_rm_long_register(rl);
  }
  else {
    INTR_RAISE_UD;
  }
}


/****************************************************************************
REMARKS:
Handles opcode 0x0f,0x23
****************************************************************************/
static void x86emuOp_mov_word_DRx_RM(u8 X86EMU_UNUSED(op2))
{
  int mod, rl, rh;

  OP_DECODE("mov dr");
  fetch_decode_modrm(&mod, &rh, &rl);
  DECODE_HEX1(rh);
  OP_DECODE(",");

  if(mod == 3) {
    M.x86.drx[rh] = *decode_rm_long_register(rl);
  }
  else {
    INTR_RAISE_UD;
  }
}


/****************************************************************************
REMARKS:
Handles opcode 0x0f,0x31
****************************************************************************/
static void x86emuOp_rdtsc(u8 X86EMU_UNUSED(op2))
{
  OP_DECODE("rdtsc");

  M.x86.R_EAX = M.x86.msr_10;
  M.x86.R_EDX = 0;
}


/****************************************************************************
REMARKS:
Handles opcode 0x0f,0x80-0x8F
****************************************************************************/
static void x86emuOp2_long_jump(u8 op2)
{
  s32 ofs;
  u32 eip;
  unsigned type = op2 & 0xf;

  OP_DECODE("j");
  decode_cond(type);

  ofs = (s16) fetch_word();
  eip = M.x86.R_EIP + ofs;
  DECODE_HEX_ADDR(eip);
  if(!MODE_DATA32) eip &= 0xffff;
  if(eval_condition(type)) M.x86.R_EIP = eip;
}


/****************************************************************************
REMARKS:
Handles opcode 0x0f,0x90-0x9F
****************************************************************************/
static void x86emuOp2_set_byte(u8 op2)
{
  int mod, rl, rh;
  u32 addr;
  u8 *reg8;
  unsigned type = op2 & 0xf;

  OP_DECODE("set");
  decode_cond(type);

  fetch_decode_modrm(&mod, &rh, &rl);

  if(mod == 3) {
    reg8 = decode_rm_byte_register(rl);
    *reg8 = eval_condition(type) ? 1 : 0;
  }
  else {
    addr = decode_rm_address(mod, rl);
    store_data_byte(addr, eval_condition(type) ? 1 : 0);
  }
}


/****************************************************************************
REMARKS:
Handles opcode 0x0f,0xa0
****************************************************************************/
static void x86emuOp2_push_FS(u8 X86EMU_UNUSED(op2))
{
  OP_DECODE("push fs");

  if(MODE_DATA32) {
    push_long(M.x86.R_FS);
  }
  else {
    push_word(M.x86.R_FS);
  }
}


/****************************************************************************
REMARKS:
Handles opcode 0x0f,0xa1
****************************************************************************/
static void x86emuOp2_pop_FS(u8 X86EMU_UNUSED(op2))
{
  OP_DECODE("pop fs");
  decode_set_seg_register(M.x86.seg + R_FS_INDEX, MODE_DATA32 ? pop_long() : pop_word());
}


/****************************************************************************
REMARKS:
Handles opcode 0x0f,0xa3
****************************************************************************/
static void x86emuOp2_bt_R(u8 X86EMU_UNUSED(op2))
{
  int mod, rl, rh;
  u32 *reg32, val, addr, mask;
  u16 *reg16;
  s32 disp;

  OP_DECODE("bt ");
  fetch_decode_modrm(&mod, &rh, &rl);

  if(mod == 3) {
    if(MODE_DATA32) {
      reg32 = decode_rm_long_register(rl);
      OP_DECODE(",");
      mask = 1 << (*decode_rm_long_register(rh) & 0x1f);
      CONDITIONAL_SET_FLAG(*reg32 & mask, F_CF);
    }
    else {
      reg16 = decode_rm_word_register(rl);
      OP_DECODE(",");
      mask = 1 << (*decode_rm_word_register(rh) & 0x0f);
      CONDITIONAL_SET_FLAG(*reg16 & mask, F_CF);
    }
  }
  else {
    addr = decode_rm_address(mod, rl);
    OP_DECODE(",");

    if(MODE_DATA32) {
      disp = *decode_rm_long_register(rh);
      mask = 1 << (disp & 0x1f);
      disp >>= 5;
      val = fetch_data_long(addr + disp);
      CONDITIONAL_SET_FLAG(val & mask, F_CF);
    }
    else {
      disp = (s16) *decode_rm_word_register(rh);
      mask = 1 << (disp & 0x0f);
      disp >>= 4;
      val = fetch_data_word(addr + disp);
      CONDITIONAL_SET_FLAG(val & mask, F_CF);
    }
  }
}


/****************************************************************************
REMARKS:
Handles opcode 0x0f,0xa4
****************************************************************************/
static void x86emuOp2_shld_IMM(u8 X86EMU_UNUSED(op2))
{
  int mod, rl, rh;
  u32 *dst32, *src32, addr, val;
  u16 *dst16, *src16;
  u8 imm;

  OP_DECODE("shld ");
  fetch_decode_modrm(&mod, &rh, &rl);

  if(mod == 3) {
    if(MODE_DATA32) {
      dst32 = decode_rm_long_register(rl);
      OP_DECODE(",");
      src32 = decode_rm_long_register(rh);
      OP_DECODE(",");
      imm = fetch_byte();
      DECODE_HEX2(imm);
      *dst32 = shld_long(*dst32, *src32, imm);
    }
    else {
      dst16 = decode_rm_word_register(rl);
      OP_DECODE(",");
      src16 = decode_rm_word_register(rh);
      OP_DECODE(",");
      imm = fetch_byte();
      DECODE_HEX2(imm);
      *dst16 = shld_word(*dst16, *src16, imm);
    }
  }
  else {
    addr = decode_rm_address(mod, rl);
    OP_DECODE(",");

    if(MODE_DATA32) {
      src32 = decode_rm_long_register(rh);
      OP_DECODE(",");
      imm = fetch_byte();
      DECODE_HEX2(imm);
      val = fetch_data_long(addr);
      val = shld_long(val, *src32, imm);
      store_data_long(addr, val);
    }
    else {
      src16 = decode_rm_word_register(rh);
      OP_DECODE(",");
      imm = fetch_byte();
      DECODE_HEX2(imm);
      val = fetch_data_word(addr);
      val = shld_word(val, *src16, imm);
      store_data_word(addr, val);
    }
  }
}


/****************************************************************************
REMARKS:
Handles opcode 0x0f,0xa5
****************************************************************************/
static void x86emuOp2_shld_CL(u8 X86EMU_UNUSED(op2))
{
  int mod, rl, rh;
  u32 *dst32, *src32, addr, val;
  u16 *dst16, *src16;


  OP_DECODE("shld ");
  fetch_decode_modrm(&mod, &rh, &rl);

  if(mod == 3) {
    if(MODE_DATA32) {
      dst32 = decode_rm_long_register(rl);
      OP_DECODE(",");
      src32 = decode_rm_long_register(rh);
      OP_DECODE(",cl");
      *dst32 = shld_long(*dst32, *src32, M.x86.R_CL);
    }
    else {
      dst16 = decode_rm_word_register(rl);
      OP_DECODE(",");
      src16 = decode_rm_word_register(rh);
      OP_DECODE(",cl");
      *dst16 = shld_word(*dst16, *src16, M.x86.R_CL);
    }
  }
  else {
    addr = decode_rm_address(mod, rl);
    OP_DECODE(",");

    if(MODE_DATA32) {
      src32 = decode_rm_long_register(rh);
      OP_DECODE(",cl");
      val = fetch_data_long(addr);
      val = shld_long(val, *src32, M.x86.R_CL);
      store_data_long(addr, val);
    }
    else {
      src16 = decode_rm_word_register(rh);
      OP_DECODE(",cl");
      val = fetch_data_word(addr);
      val = shld_word(val, *src16, M.x86.R_CL);
      store_data_word(addr, val);
    }
  }
}


/****************************************************************************
REMARKS:
Handles opcode 0x0f,0xa8
****************************************************************************/
static void x86emuOp2_push_GS(u8 X86EMU_UNUSED(op2))
{
  OP_DECODE("push gs");

  if(MODE_DATA32) {
    push_long(M.x86.R_GS);
  }
  else {
    push_word(M.x86.R_GS);
  }
}


/****************************************************************************
REMARKS:
Handles opcode 0x0f,0xa9
****************************************************************************/
static void x86emuOp2_pop_GS(u8 X86EMU_UNUSED(op2))
{
  OP_DECODE("pop gs");
  decode_set_seg_register(M.x86.seg + R_GS_INDEX, MODE_DATA32 ? pop_long() : pop_word());
}


/****************************************************************************
REMARKS:
Handles opcode 0x0f,0xab
****************************************************************************/
static void x86emuOp2_bts_R(u8 X86EMU_UNUSED(op2))
{
  int mod, rl, rh;
  u32 *reg32, val, addr, mask;
  u16 *reg16;
  s32 disp;

  OP_DECODE("bts ");
  fetch_decode_modrm(&mod, &rh, &rl);

  if(mod == 3) {
    if(MODE_DATA32) {
      reg32 = decode_rm_long_register(rl);
      OP_DECODE(",");
      mask = 1 << (*decode_rm_long_register(rh) & 0x1f);
      CONDITIONAL_SET_FLAG(*reg32 & mask, F_CF);
      *reg32 |= mask;
    }
    else {
      reg16 = decode_rm_word_register(rl);
      OP_DECODE(",");
      mask = 1 << (*decode_rm_word_register(rh) & 0x0f);
      CONDITIONAL_SET_FLAG(*reg16 & mask, F_CF);
      *reg16 |= mask;
    }
  }
  else {
    addr = decode_rm_address(mod, rl);
    OP_DECODE(",");

    if(MODE_DATA32) {
      disp = *decode_rm_long_register(rh);
      mask = 1 << (disp & 0x1f);
      disp >>= 5;
      val = fetch_data_long(addr + disp);
      CONDITIONAL_SET_FLAG(val & mask, F_CF);
      store_data_long(addr + disp, val | mask);
    }
    else {
      disp = (s16) *decode_rm_word_register(rh);
      mask = 1 << (disp & 0x0f);
      disp >>= 5;
      val = fetch_data_word(addr + disp);
      CONDITIONAL_SET_FLAG(val & mask, F_CF);
      store_data_word(addr + disp, val | mask);
    }
  }
}


/****************************************************************************
REMARKS:
Handles opcode 0x0f,0xac
****************************************************************************/
static void x86emuOp2_shrd_IMM(u8 X86EMU_UNUSED(op2))
{
  int mod, rl, rh;
  u32 *dst32, *src32, addr, val;
  u16 *dst16, *src16;
  u8 imm;

  OP_DECODE("shrd ");
  fetch_decode_modrm(&mod, &rh, &rl);

  if(mod == 3) {
    if(MODE_DATA32) {
      dst32 = decode_rm_long_register(rl);
      OP_DECODE(",");
      src32 = decode_rm_long_register(rh);
      OP_DECODE(",");
      imm = fetch_byte();
      DECODE_HEX2(imm);
      *dst32 = shrd_long(*dst32, *src32, imm);
    }
    else {
      dst16 = decode_rm_word_register(rl);
      OP_DECODE(",");
      src16 = decode_rm_word_register(rh);
      OP_DECODE(",");
      imm = fetch_byte();
      DECODE_HEX2(imm);
      *dst16 = shrd_word(*dst16, *src16, imm);
    }
  }
  else {
    addr = decode_rm_address(mod, rl);
    OP_DECODE(",");

    if(MODE_DATA32) {
      src32 = decode_rm_long_register(rh);
      OP_DECODE(",");
      imm = fetch_byte();
      DECODE_HEX2(imm);
      val = fetch_data_long(addr);
      val = shrd_long(val, *src32, imm);
      store_data_long(addr, val);
    }
    else {
      src16 = decode_rm_word_register(rh);
      OP_DECODE(",");
      imm = fetch_byte();
      DECODE_HEX2(imm);
      val = fetch_data_word(addr);
      val = shrd_word(val, *src16, imm);
      store_data_word(addr, val);
    }
  }
}


/****************************************************************************
REMARKS:
Handles opcode 0x0f,0xad
****************************************************************************/
static void x86emuOp2_shrd_CL(u8 X86EMU_UNUSED(op2))
{
  int mod, rl, rh;
  u32 *dst32, *src32, addr, val;
  u16 *dst16, *src16;


  OP_DECODE("shrd ");
  fetch_decode_modrm(&mod, &rh, &rl);

  if(mod == 3) {
    if(MODE_DATA32) {
      dst32 = decode_rm_long_register(rl);
      OP_DECODE(",");
      src32 = decode_rm_long_register(rh);
      OP_DECODE(",cl");
      *dst32 = shrd_long(*dst32, *src32, M.x86.R_CL);
    }
    else {
      dst16 = decode_rm_word_register(rl);
      OP_DECODE(",");
      src16 = decode_rm_word_register(rh);
      OP_DECODE(",cl");
      *dst16 = shrd_word(*dst16, *src16, M.x86.R_CL);
    }
  }
  else {
    addr = decode_rm_address(mod, rl);
    OP_DECODE(",");

    if(MODE_DATA32) {
      src32 = decode_rm_long_register(rh);
      OP_DECODE(",cl");
      val = fetch_data_long(addr);
      val = shrd_long(val, *src32, M.x86.R_CL);
      store_data_long(addr, val);
    }
    else {
      src16 = decode_rm_word_register(rh);
      OP_DECODE(",cl");
      val = fetch_data_word(addr);
      val = shrd_word(val, *src16, M.x86.R_CL);
      store_data_word(addr, val);
    }
  }
}


/****************************************************************************
REMARKS:
Handles opcode 0x0f,0xaf
****************************************************************************/
static void x86emuOp2_imul_R_RM(u8 X86EMU_UNUSED(op2))
{
  int mod, rl, rh;
  u32 *src32, *dst32, val, addr, res_lo, res_hi;
  u16 *src16, *dst16;
  
  OP_DECODE("imul ");
  fetch_decode_modrm(&mod, &rh, &rl);

  if(mod == 3) {
    if(MODE_DATA32) {
      dst32 = decode_rm_long_register(rh);
      OP_DECODE(",");
      src32 = decode_rm_long_register(rl);
      imul_long_direct(&res_lo, &res_hi, *dst32, *src32);
      if(res_hi != 0) {
        SET_FLAG(F_CF);
        SET_FLAG(F_OF);
      }
      else {
        CLEAR_FLAG(F_CF);
        CLEAR_FLAG(F_OF);
      }
      *dst32= res_lo;
    }
    else {
      dst16 = decode_rm_word_register(rh);
      OP_DECODE(",");
      src16 = decode_rm_word_register(rl);
      res_lo = (s32) ((s16) *dst16 * (s16) *src16);
      if(res_lo > 0xffff) {
        SET_FLAG(F_CF);
        SET_FLAG(F_OF);
      }
      else {
        CLEAR_FLAG(F_CF);
        CLEAR_FLAG(F_OF);
      }
      *dst16 = res_lo;
    }
  }
  else {
    if(MODE_DATA32) {
      dst32 = decode_rm_long_register(rh);
      OP_DECODE(",");
      addr = decode_rm_address(mod, rl);
      val = fetch_data_long(addr);
      imul_long_direct(&res_lo, &res_hi, *dst32, val);
      if(res_hi != 0) {
        SET_FLAG(F_CF);
        SET_FLAG(F_OF);
      }
      else {
        CLEAR_FLAG(F_CF);
        CLEAR_FLAG(F_OF);
      }
      *dst32 = res_lo;
    }
    else {
      dst16 = decode_rm_word_register(rh);
      OP_DECODE(",");
      addr = decode_rm_address(mod, rl);
      val = fetch_data_word(addr);
      res_lo = (s32) ((s16) *dst16 * (s16) val);
      if(res_lo > 0xffff) {
        SET_FLAG(F_CF);
        SET_FLAG(F_OF);
      }
      else {
        CLEAR_FLAG(F_CF);
        CLEAR_FLAG(F_OF);
      }
      *dst16 = res_lo;
    }
  }
}


/****************************************************************************
REMARKS:
Handles opcode 0x0f,0xb2
****************************************************************************/
static void x86emuOp2_lss_R_IMM(u8 X86EMU_UNUSED(op2))
{
  int mod, rh, rl;
  u16 *reg16;
  u32 *reg32, addr;

  OP_DECODE("lss ");
  fetch_decode_modrm(&mod, &rh, &rl);
  if(mod == 3) {
    INTR_RAISE_UD;
  }
  else {
    if(MODE_DATA32){
      reg32 = decode_rm_long_register(rh);
      OP_DECODE(",");
      addr = decode_rm_address(mod, rl);
      *reg32 = fetch_data_long(addr);
      addr += 4;
    }
    else {
      reg16 = decode_rm_word_register(rh);
      OP_DECODE(",");
      addr = decode_rm_address(mod, rl);
      *reg16 = fetch_data_word(addr);
      addr += 2;
    }
    decode_set_seg_register(M.x86.seg + R_SS_INDEX, fetch_data_word(addr));
  }
}


/****************************************************************************
REMARKS:
Handles opcode 0x0f,0xb3
****************************************************************************/
static void x86emuOp2_btr_R(u8 X86EMU_UNUSED(op2))
{
  int mod, rl, rh;
  u32 *reg32, val, addr, mask;
  u16 *reg16;
  s32 disp;

  OP_DECODE("btr ");
  fetch_decode_modrm(&mod, &rh, &rl);

  if(mod == 3) {
    if(MODE_DATA32) {
      reg32 = decode_rm_long_register(rl);
      OP_DECODE(",");
      mask = 1 << (*decode_rm_long_register(rh) & 0x1f);
      CONDITIONAL_SET_FLAG(*reg32 & mask, F_CF);
      *reg32 &= ~mask;
    }
    else {
      reg16 = decode_rm_word_register(rl);
      OP_DECODE(",");
      mask = 1 << (*decode_rm_word_register(rh) & 0x0f);
      CONDITIONAL_SET_FLAG(*reg16 & mask, F_CF);
      *reg16 &= ~mask;
    }
  }
  else {
    addr = decode_rm_address(mod, rl);
    OP_DECODE(",");

    if(MODE_DATA32) {
      disp = *decode_rm_long_register(rh);
      mask = 1 << (disp & 0x1f);
      disp >>= 5;
      val = fetch_data_long(addr + disp);
      CONDITIONAL_SET_FLAG(val & mask, F_CF);
      store_data_long(addr + disp, val & ~mask);
    }
    else {
      disp = (s16) *decode_rm_word_register(rh);
      mask = 1 << (disp & 0x0f);
      disp >>= 5;
      val = fetch_data_word(addr + disp);
      CONDITIONAL_SET_FLAG(val & mask, F_CF);
      store_data_word(addr + disp, val & ~mask);
    }
  }
}


/****************************************************************************
REMARKS:
Handles opcode 0x0f,0xb4
****************************************************************************/
static void x86emuOp2_lfs_R_IMM(u8 X86EMU_UNUSED(op2))
{
  int mod, rh, rl;
  u16 *reg16;
  u32 *reg32, addr;

  OP_DECODE("lfs ");
  fetch_decode_modrm(&mod, &rh, &rl);
  if(mod == 3) {
    INTR_RAISE_UD;
  }
  else {
    if(MODE_DATA32){
      reg32 = decode_rm_long_register(rh);
      OP_DECODE(",");
      addr = decode_rm_address(mod, rl);
      *reg32 = fetch_data_long(addr);
      addr += 4;
    }
    else {
      reg16 = decode_rm_word_register(rh);
      OP_DECODE(",");
      addr = decode_rm_address(mod, rl);
      *reg16 = fetch_data_word(addr);
      addr += 2;
    }
    decode_set_seg_register(M.x86.seg + R_FS_INDEX, fetch_data_word(addr));
  }
}


/****************************************************************************
REMARKS:
Handles opcode 0x0f,0xb5
****************************************************************************/
static void x86emuOp2_lgs_R_IMM(u8 X86EMU_UNUSED(op2))
{
  int mod, rh, rl;
  u16 *reg16;
  u32 *reg32, addr;

  OP_DECODE("lgs ");
  fetch_decode_modrm(&mod, &rh, &rl);
  if(mod == 3) {
    INTR_RAISE_UD;
  }
  else {
    if(MODE_DATA32){
      reg32 = decode_rm_long_register(rh);
      OP_DECODE(",");
      addr = decode_rm_address(mod, rl);
      *reg32 = fetch_data_long(addr);
      addr += 4;
    }
    else {
      reg16 = decode_rm_word_register(rh);
      OP_DECODE(",");
      addr = decode_rm_address(mod, rl);
      *reg16 = fetch_data_word(addr);
      addr += 2;
    }
    decode_set_seg_register(M.x86.seg + R_GS_INDEX, fetch_data_word(addr));
  }
}


/****************************************************************************
REMARKS:
Handles opcode 0x0f,0xb6
****************************************************************************/
static void x86emuOp2_movzx_byte_R_RM(u8 X86EMU_UNUSED(op2))
{
  int mod, rl, rh;
  u32 *reg32, addr, val;
  u16 *reg16;
  u8 *reg8;

  OP_DECODE("movzx ");
  fetch_decode_modrm(&mod, &rh, &rl);

  if(mod == 3) {
    if(MODE_DATA32) {
      reg32 = decode_rm_long_register(rh);
      OP_DECODE(",");
      reg8 = decode_rm_byte_register(rl);
      *reg32 = *reg8;
    }
    else {
      reg16 = decode_rm_word_register(rh);
      OP_DECODE(",");
      reg8 = decode_rm_byte_register(rl);
      *reg16 = *reg8;
    }
  }
  else {
    if(MODE_DATA32) {
      reg32 = decode_rm_long_register(rh);
      OP_DECODE(",byte ");
      addr = decode_rm_address(mod, rl);
      val = fetch_data_byte(addr);
      *reg32 = val;
    }
    else {
      reg16 = decode_rm_word_register(rh);
      OP_DECODE(",byte ");
      addr = decode_rm_address(mod, rl);
      val = fetch_data_byte(addr);
      *reg16 = val;
    }
  }
}


/****************************************************************************
REMARKS:
Handles opcode 0x0f,0xb7
****************************************************************************/
static void x86emuOp2_movzx_word_R_RM(u8 X86EMU_UNUSED(op2))
{
  int mod, rl, rh;
  u32 *reg32, addr, val;
  u16 *reg16;

  OP_DECODE("movzx ");
  fetch_decode_modrm(&mod, &rh, &rl);

  if(mod == 3) {
    if(MODE_DATA32) {
      reg32 = decode_rm_long_register(rh);
      OP_DECODE(",");
      reg16 = decode_rm_word_register(rl);
      *reg32 = *reg16;
    }
    else {
      reg16 = decode_rm_word_register(rh);
      OP_DECODE(",");
      *reg16 = *decode_rm_word_register(rl);
    }
  }
  else {
    if(MODE_DATA32) {
      reg32 = decode_rm_long_register(rh);
      OP_DECODE(",word ");
      addr = decode_rm_address(mod, rl);
      val = fetch_data_word(addr);
      *reg32 = val;
    }
    else {
      reg16 = decode_rm_word_register(rh);
      OP_DECODE(",word ");
      addr = decode_rm_address(mod, rl);
      val = fetch_data_word(addr);
      *reg16 = val;
    }
  }
}


/****************************************************************************
REMARKS:
Handles opcode 0x0f,0xba
****************************************************************************/
static void x86emuOp2_btX_I(u8 X86EMU_UNUSED(op2))
{
  int mod, rl, rh;
  u32 *reg32, val, addr, mask;
  u16 *reg16;
  u8 imm;

  fetch_decode_modrm(&mod, &rh, &rl);

  switch (rh) {
    case 4:
      OP_DECODE("bt ");
      break;
    case 5:
      OP_DECODE("bts ");
      break;
    case 6:
      OP_DECODE("btr ");
      break;
    case 7:
      OP_DECODE("btc ");
      break;
    default:
      INTR_RAISE_UD;
      return;
  }

  if(mod == 3) {
    if(MODE_DATA32) {
      reg32 = decode_rm_long_register(rl);
      OP_DECODE(",");
      imm = fetch_byte();
      DECODE_HEX2(imm);
      mask = 1 << (imm & 0x1f);
      CONDITIONAL_SET_FLAG(*reg32 & mask, F_CF);
      switch(rh) {
        case 5:
          *reg32 |= mask;
          break;
        case 6:
          *reg32 &= ~mask;
          break;
        case 7:
          *reg32 ^= mask;
          break;
      }
    }
    else {
      reg16 = decode_rm_word_register(rl);
      OP_DECODE(",");
      imm = fetch_byte();
      DECODE_HEX2(imm);
      mask = 1 << (imm & 0x1f);
      CONDITIONAL_SET_FLAG(*reg16 & mask, F_CF);
      switch(rh) {
        case 5:
          *reg16 |= mask;
          break;
        case 6:
          *reg16 &= ~mask;
          break;
        case 7:
          *reg16 ^= mask;
          break;
      }
    }
  }
  else {
    addr = decode_rm_address(mod, rl);
    OP_DECODE(",");
    imm = fetch_byte();
    DECODE_HEX2(imm);

    if(MODE_DATA32) {
      mask = 1 << (imm & 0x1f);
      val = fetch_data_long(addr);
      CONDITIONAL_SET_FLAG(val & mask, F_CF);
      switch(rh) {
        case 5:
          store_data_long(addr, val | mask);
          break;
        case 6:
          store_data_long(addr, val & ~mask);
          break;
        case 7:
          store_data_long(addr, val ^ mask);
          break;
        }
    }
    else {
      mask = 1 << (imm & 0x0f);
      val = fetch_data_word(addr);
      CONDITIONAL_SET_FLAG(val & mask, F_CF);
      switch(rh) {
        case 5:
          store_data_word(addr, val | mask);
          break;
        case 6:
          store_data_word(addr, val & ~mask);
          break;
        case 7:
          store_data_word(addr, val ^ mask);
          break;
      }
    }
  }
}


/****************************************************************************
REMARKS:
Handles opcode 0x0f,0xbb
****************************************************************************/
static void x86emuOp2_btc_R(u8 X86EMU_UNUSED(op2))
{
  int mod, rl, rh;
  u32 *reg32, val, addr, mask;
  u16 *reg16;
  s32 disp;

  OP_DECODE("btc ");
  fetch_decode_modrm(&mod, &rh, &rl);

  if(mod == 3) {
    if(MODE_DATA32) {
      reg32 = decode_rm_long_register(rl);
      OP_DECODE(",");
      mask = 1 << (*decode_rm_long_register(rh) & 0x1f);
      CONDITIONAL_SET_FLAG(*reg32 & mask, F_CF);
      *reg32 ^= mask;
    }
    else {
      reg16 = decode_rm_word_register(rl);
      OP_DECODE(",");
      mask = 1 << (*decode_rm_word_register(rh) & 0x0f);
      CONDITIONAL_SET_FLAG(*reg16 & mask, F_CF);
      *reg16 ^= mask;
    }
  }
  else {
    addr = decode_rm_address(mod, rl);
    OP_DECODE(",");

    if(MODE_DATA32) {
      disp = *decode_rm_long_register(rh);
      mask = 1 << (disp & 0x1f);
      disp >>= 5;
      val = fetch_data_long(addr + disp);
      CONDITIONAL_SET_FLAG(val & mask, F_CF);
      store_data_long(addr + disp, val ^ mask);
    }
    else {
      disp = (s16) *decode_rm_word_register(rh);
      mask = 1 << (disp & 0x0f);
      disp >>= 5;
      val = fetch_data_word(addr + disp);
      CONDITIONAL_SET_FLAG(val & mask, F_CF);
      store_data_word(addr + disp, val ^ mask);
    }
  }
}


/****************************************************************************
REMARKS:
Handles opcode 0x0f,0xbc
****************************************************************************/
static void x86emuOp2_bsf(u8 X86EMU_UNUSED(op2))
{
  int mod, rl, rh;
  u32 *reg32, addr, val, cnt;
  u16 *reg16;

  OP_DECODE("bsf");
  fetch_decode_modrm(&mod, &rh, &rl);

  if(mod == 3) {
    if(MODE_DATA32) {
      val = *decode_rm_long_register(rl);
      OP_DECODE(",");
      reg32 = decode_rm_long_register(rh);
      CONDITIONAL_SET_FLAG(val == 0, F_ZF);
      for(cnt = 0; cnt < 32; cnt++) if((val >> cnt) & 1) break;
      *reg32 = cnt;
    }
    else {
      val = *decode_rm_word_register(rl);
      OP_DECODE(",");
      reg16 = decode_rm_word_register(rh);
      CONDITIONAL_SET_FLAG(val == 0, F_ZF);
      for(cnt = 0; cnt < 16; cnt++) if((val >> cnt) & 1) break;
      *reg16 = cnt;
    }
  }
  else {
    addr = decode_rm_address(mod, rl);
    OP_DECODE(",");

    if(MODE_DATA32) {
      reg32 = decode_rm_long_register(rh);
      val = fetch_data_long(addr);
      CONDITIONAL_SET_FLAG(val == 0, F_ZF);
      for(cnt = 0; cnt < 32; cnt++) if((val >> cnt) & 1) break;
      *reg32 = cnt;
    }
    else {
      reg16 = decode_rm_word_register(rh);
      val = fetch_data_word(addr);
      CONDITIONAL_SET_FLAG(val == 0, F_ZF);
      for(cnt = 0; cnt < 16; cnt++) if((val >> cnt) & 1) break;
      *reg16 = cnt;
    }
  }
}


/****************************************************************************
REMARKS:
Handles opcode 0x0f,0xbd
****************************************************************************/
static void x86emuOp2_bsr(u8 X86EMU_UNUSED(op2))
{
  int mod, rl, rh;
  u32 *reg32, addr, val, cnt;
  u16 *reg16;

  OP_DECODE("bsr");
  fetch_decode_modrm(&mod, &rh, &rl);

  if(mod == 3) {
    if(MODE_DATA32) {
      val = *decode_rm_long_register(rl);
      OP_DECODE(",");
      reg32 = decode_rm_long_register(rh);
      CONDITIONAL_SET_FLAG(val == 0, F_ZF);
      for(cnt = 31; cnt > 0; cnt--) if((val >> cnt) & 1) break;
      *reg32 = cnt;
    }
    else {
      val = *decode_rm_word_register(rl);
      OP_DECODE(",");
      reg16 = decode_rm_word_register(rh);
      CONDITIONAL_SET_FLAG(val == 0, F_ZF);
      for(cnt = 15; cnt > 0; cnt--) if((val >> cnt) & 1) break;
      *reg16 = cnt;
    }
  }
  else {
    addr = decode_rm_address(mod, rl);
    OP_DECODE(",");

    if(MODE_DATA32) {
      reg32 = decode_rm_long_register(rh);
      val = fetch_data_long(addr);
      CONDITIONAL_SET_FLAG(val == 0, F_ZF);
      for(cnt = 31; cnt > 0; cnt--) if((val >> cnt) & 1) break;
      *reg32 = cnt;
    }
    else {
      reg16 = decode_rm_word_register(rh);
      val = fetch_data_word(addr);
      CONDITIONAL_SET_FLAG(val == 0, F_ZF);
      for(cnt = 15; cnt > 0; cnt--) if((val >> cnt) & 1) break;
      *reg16 = cnt;
    }
  }
}


/****************************************************************************
REMARKS:
Handles opcode 0x0f,0xbe
****************************************************************************/
static void x86emuOp2_movsx_byte_R_RM(u8 X86EMU_UNUSED(op2))
{
  int mod, rl, rh;
  u32 *reg32, addr, val;
  u16 *reg16;
  u8 *reg8;

  OP_DECODE("movsx ");
  fetch_decode_modrm(&mod, &rh, &rl);

  if(mod == 3) {
    if(MODE_DATA32) {
      reg32 = decode_rm_long_register(rh);
      OP_DECODE(",");
      reg8 = decode_rm_byte_register(rl);
      *reg32 = (s8) *reg8;
    }
    else {
      reg16 = decode_rm_word_register(rh);
      OP_DECODE(",");
      reg8 = decode_rm_byte_register(rl);
      *reg16 = (s8) *reg8;
    }
  }
  else {
    if(MODE_DATA32) {
      reg32 = decode_rm_long_register(rh);
      OP_DECODE(",byte ");
      addr = decode_rm_address(mod, rl);
      val = (s8) fetch_data_byte(addr);
      *reg32 = val;
    }
    else {
      reg16 = decode_rm_word_register(rh);
      OP_DECODE(",byte ");
      addr = decode_rm_address(mod, rl);
      val = (s8) fetch_data_byte(addr);
      *reg16 = val;
    }
  }
}


/****************************************************************************
REMARKS:
Handles opcode 0x0f,0xbf
****************************************************************************/
static void x86emuOp2_movsx_word_R_RM(u8 X86EMU_UNUSED(op2))
{
  int mod, rl, rh;
  u32 *reg32, addr, val;
  u16 *reg16;

  OP_DECODE("movsx ");
  fetch_decode_modrm(&mod, &rh, &rl);

  if(mod == 3) {
    if(MODE_DATA32) {
      reg32 = decode_rm_long_register(rh);
      OP_DECODE(",");
      reg16 = decode_rm_word_register(rl);
      *reg32 = (s16) *reg16;
    }
    else {
      reg16 = decode_rm_word_register(rh);
      OP_DECODE(",");
      *reg16 = (s16) *decode_rm_word_register(rl);
    }
  }
  else {
    if(MODE_DATA32) {
      reg32 = decode_rm_long_register(rh);
      OP_DECODE(",word ");
      addr = decode_rm_address(mod, rl);
      val = (s16) fetch_data_word(addr);
      *reg32 = val;
    }
    else {
      reg16 = decode_rm_word_register(rh);
      OP_DECODE(",word ");
      addr = decode_rm_address(mod, rl);
      val = (s16) fetch_data_word(addr);
      *reg16 = val;
    }
  }
}


/***************************************************************************
 * Double byte operation code table:
 **************************************************************************/
void (*x86emu_optab2[256])(u8) =
{
  /*  0x00 */ x86emuOp2_opc_00,      /* Group F (ring 0 PM)      */
  /*  0x01 */ x86emuOp2_opc_01,      /* Group G (ring 0 PM)      */
  /*  0x02 */ x86emuOp2_illegal_op,  /* lar (ring 0 PM)          */
  /*  0x03 */ x86emuOp2_illegal_op,  /* lsl (ring 0 PM)          */
  /*  0x04 */ x86emuOp2_illegal_op,
  /*  0x05 */ x86emuOp2_illegal_op,  /* loadall (undocumented)   */
  /*  0x06 */ x86emuOp2_illegal_op,  /* clts (ring 0 PM)         */
  /*  0x07 */ x86emuOp2_illegal_op,  /* loadall (undocumented)   */
  /*  0x08 */ x86emuOp2_illegal_op,  /* invd (ring 0 PM)         */
  /*  0x09 */ x86emuOp2_illegal_op,  /* wbinvd (ring 0 PM)       */
  /*  0x0a */ x86emuOp2_illegal_op,
  /*  0x0b */ x86emuOp2_illegal_op,
  /*  0x0c */ x86emuOp2_illegal_op,
  /*  0x0d */ x86emuOp2_illegal_op,
  /*  0x0e */ x86emuOp2_illegal_op,
  /*  0x0f */ x86emuOp2_illegal_op,

  /*  0x10 */ x86emuOp2_illegal_op,
  /*  0x11 */ x86emuOp2_illegal_op,
  /*  0x12 */ x86emuOp2_illegal_op,
  /*  0x13 */ x86emuOp2_illegal_op,
  /*  0x14 */ x86emuOp2_illegal_op,
  /*  0x15 */ x86emuOp2_illegal_op,
  /*  0x16 */ x86emuOp2_illegal_op,
  /*  0x17 */ x86emuOp2_illegal_op,
  /*  0x18 */ x86emuOp2_illegal_op,
  /*  0x19 */ x86emuOp2_illegal_op,
  /*  0x1a */ x86emuOp2_illegal_op,
  /*  0x1b */ x86emuOp2_illegal_op,
  /*  0x1c */ x86emuOp2_illegal_op,
  /*  0x1d */ x86emuOp2_illegal_op,
  /*  0x1e */ x86emuOp2_illegal_op,
  /*  0x1f */ x86emuOp2_illegal_op,

  /*  0x20 */ x86emuOp_mov_word_RM_CRx,
  /*  0x21 */ x86emuOp_mov_word_RM_DRx,
  /*  0x22 */ x86emuOp_mov_word_CRx_RM,
  /*  0x23 */ x86emuOp_mov_word_DRx_RM,
  /*  0x24 */ x86emuOp2_illegal_op,  /* mov reg32,treg (ring 0 PM) */
  /*  0x25 */ x86emuOp2_illegal_op,
  /*  0x26 */ x86emuOp2_illegal_op,  /* mov treg,reg32 (ring 0 PM) */
  /*  0x27 */ x86emuOp2_illegal_op,
  /*  0x28 */ x86emuOp2_illegal_op,
  /*  0x29 */ x86emuOp2_illegal_op,
  /*  0x2a */ x86emuOp2_illegal_op,
  /*  0x2b */ x86emuOp2_illegal_op,
  /*  0x2c */ x86emuOp2_illegal_op,
  /*  0x2d */ x86emuOp2_illegal_op,
  /*  0x2e */ x86emuOp2_illegal_op,
  /*  0x2f */ x86emuOp2_illegal_op,

  /*  0x30 */ x86emuOp2_illegal_op,
  /*  0x31 */ x86emuOp_rdtsc,
  /*  0x32 */ x86emuOp2_illegal_op,
  /*  0x33 */ x86emuOp2_illegal_op,
  /*  0x34 */ x86emuOp2_illegal_op,
  /*  0x35 */ x86emuOp2_illegal_op,
  /*  0x36 */ x86emuOp2_illegal_op,
  /*  0x37 */ x86emuOp2_illegal_op,
  /*  0x38 */ x86emuOp2_illegal_op,
  /*  0x39 */ x86emuOp2_illegal_op,
  /*  0x3a */ x86emuOp2_illegal_op,
  /*  0x3b */ x86emuOp2_illegal_op,
  /*  0x3c */ x86emuOp2_illegal_op,
  /*  0x3d */ x86emuOp2_illegal_op,
  /*  0x3e */ x86emuOp2_illegal_op,
  /*  0x3f */ x86emuOp2_illegal_op,

  /*  0x40 */ x86emuOp2_illegal_op,
  /*  0x41 */ x86emuOp2_illegal_op,
  /*  0x42 */ x86emuOp2_illegal_op,
  /*  0x43 */ x86emuOp2_illegal_op,
  /*  0x44 */ x86emuOp2_illegal_op,
  /*  0x45 */ x86emuOp2_illegal_op,
  /*  0x46 */ x86emuOp2_illegal_op,
  /*  0x47 */ x86emuOp2_illegal_op,
  /*  0x48 */ x86emuOp2_illegal_op,
  /*  0x49 */ x86emuOp2_illegal_op,
  /*  0x4a */ x86emuOp2_illegal_op,
  /*  0x4b */ x86emuOp2_illegal_op,
  /*  0x4c */ x86emuOp2_illegal_op,
  /*  0x4d */ x86emuOp2_illegal_op,
  /*  0x4e */ x86emuOp2_illegal_op,
  /*  0x4f */ x86emuOp2_illegal_op,

  /*  0x50 */ x86emuOp2_illegal_op,
  /*  0x51 */ x86emuOp2_illegal_op,
  /*  0x52 */ x86emuOp2_illegal_op,
  /*  0x53 */ x86emuOp2_illegal_op,
  /*  0x54 */ x86emuOp2_illegal_op,
  /*  0x55 */ x86emuOp2_illegal_op,
  /*  0x56 */ x86emuOp2_illegal_op,
  /*  0x57 */ x86emuOp2_illegal_op,
  /*  0x58 */ x86emuOp2_illegal_op,
  /*  0x59 */ x86emuOp2_illegal_op,
  /*  0x5a */ x86emuOp2_illegal_op,
  /*  0x5b */ x86emuOp2_illegal_op,
  /*  0x5c */ x86emuOp2_illegal_op,
  /*  0x5d */ x86emuOp2_illegal_op,
  /*  0x5e */ x86emuOp2_illegal_op,
  /*  0x5f */ x86emuOp2_illegal_op,

  /*  0x60 */ x86emuOp2_illegal_op,
  /*  0x61 */ x86emuOp2_illegal_op,
  /*  0x62 */ x86emuOp2_illegal_op,
  /*  0x63 */ x86emuOp2_illegal_op,
  /*  0x64 */ x86emuOp2_illegal_op,
  /*  0x65 */ x86emuOp2_illegal_op,
  /*  0x66 */ x86emuOp2_illegal_op,
  /*  0x67 */ x86emuOp2_illegal_op,
  /*  0x68 */ x86emuOp2_illegal_op,
  /*  0x69 */ x86emuOp2_illegal_op,
  /*  0x6a */ x86emuOp2_illegal_op,
  /*  0x6b */ x86emuOp2_illegal_op,
  /*  0x6c */ x86emuOp2_illegal_op,
  /*  0x6d */ x86emuOp2_illegal_op,
  /*  0x6e */ x86emuOp2_illegal_op,
  /*  0x6f */ x86emuOp2_illegal_op,

  /*  0x70 */ x86emuOp2_illegal_op,
  /*  0x71 */ x86emuOp2_illegal_op,
  /*  0x72 */ x86emuOp2_illegal_op,
  /*  0x73 */ x86emuOp2_illegal_op,
  /*  0x74 */ x86emuOp2_illegal_op,
  /*  0x75 */ x86emuOp2_illegal_op,
  /*  0x76 */ x86emuOp2_illegal_op,
  /*  0x77 */ x86emuOp2_illegal_op,
  /*  0x78 */ x86emuOp2_illegal_op,
  /*  0x79 */ x86emuOp2_illegal_op,
  /*  0x7a */ x86emuOp2_illegal_op,
  /*  0x7b */ x86emuOp2_illegal_op,
  /*  0x7c */ x86emuOp2_illegal_op,
  /*  0x7d */ x86emuOp2_illegal_op,
  /*  0x7e */ x86emuOp2_illegal_op,
  /*  0x7f */ x86emuOp2_illegal_op,

  /*  0x80 */ x86emuOp2_long_jump,
  /*  0x81 */ x86emuOp2_long_jump,
  /*  0x82 */ x86emuOp2_long_jump,
  /*  0x83 */ x86emuOp2_long_jump,
  /*  0x84 */ x86emuOp2_long_jump,
  /*  0x85 */ x86emuOp2_long_jump,
  /*  0x86 */ x86emuOp2_long_jump,
  /*  0x87 */ x86emuOp2_long_jump,
  /*  0x88 */ x86emuOp2_long_jump,
  /*  0x89 */ x86emuOp2_long_jump,
  /*  0x8a */ x86emuOp2_long_jump,
  /*  0x8b */ x86emuOp2_long_jump,
  /*  0x8c */ x86emuOp2_long_jump,
  /*  0x8d */ x86emuOp2_long_jump,
  /*  0x8e */ x86emuOp2_long_jump,
  /*  0x8f */ x86emuOp2_long_jump,

  /*  0x90 */ x86emuOp2_set_byte,
  /*  0x91 */ x86emuOp2_set_byte,
  /*  0x92 */ x86emuOp2_set_byte,
  /*  0x93 */ x86emuOp2_set_byte,
  /*  0x94 */ x86emuOp2_set_byte,
  /*  0x95 */ x86emuOp2_set_byte,
  /*  0x96 */ x86emuOp2_set_byte,
  /*  0x97 */ x86emuOp2_set_byte,
  /*  0x98 */ x86emuOp2_set_byte,
  /*  0x99 */ x86emuOp2_set_byte,
  /*  0x9a */ x86emuOp2_set_byte,
  /*  0x9b */ x86emuOp2_set_byte,
  /*  0x9c */ x86emuOp2_set_byte,
  /*  0x9d */ x86emuOp2_set_byte,
  /*  0x9e */ x86emuOp2_set_byte,
  /*  0x9f */ x86emuOp2_set_byte,

  /*  0xa0 */ x86emuOp2_push_FS,
  /*  0xa1 */ x86emuOp2_pop_FS,
  /*  0xa2 */ x86emuOp2_illegal_op,
  /*  0xa3 */ x86emuOp2_bt_R,
  /*  0xa4 */ x86emuOp2_shld_IMM,
  /*  0xa5 */ x86emuOp2_shld_CL,
  /*  0xa6 */ x86emuOp2_illegal_op,
  /*  0xa7 */ x86emuOp2_illegal_op,
  /*  0xa8 */ x86emuOp2_push_GS,
  /*  0xa9 */ x86emuOp2_pop_GS,
  /*  0xaa */ x86emuOp2_illegal_op,
  /*  0xab */ x86emuOp2_bts_R,
  /*  0xac */ x86emuOp2_shrd_IMM,
  /*  0xad */ x86emuOp2_shrd_CL,
  /*  0xae */ x86emuOp2_illegal_op,
  /*  0xaf */ x86emuOp2_imul_R_RM,

  /*  0xb0 */ x86emuOp2_illegal_op,  /* TODO: cmpxchg */
  /*  0xb1 */ x86emuOp2_illegal_op,  /* TODO: cmpxchg */
  /*  0xb2 */ x86emuOp2_lss_R_IMM,
  /*  0xb3 */ x86emuOp2_btr_R,
  /*  0xb4 */ x86emuOp2_lfs_R_IMM,
  /*  0xb5 */ x86emuOp2_lgs_R_IMM,
  /*  0xb6 */ x86emuOp2_movzx_byte_R_RM,
  /*  0xb7 */ x86emuOp2_movzx_word_R_RM,
  /*  0xb8 */ x86emuOp2_illegal_op,
  /*  0xb9 */ x86emuOp2_illegal_op,
  /*  0xba */ x86emuOp2_btX_I,
  /*  0xbb */ x86emuOp2_btc_R,
  /*  0xbc */ x86emuOp2_bsf,
  /*  0xbd */ x86emuOp2_bsr,
  /*  0xbe */ x86emuOp2_movsx_byte_R_RM,
  /*  0xbf */ x86emuOp2_movsx_word_R_RM,

  /*  0xc0 */ x86emuOp2_illegal_op,  /* TODO: xadd */
  /*  0xc1 */ x86emuOp2_illegal_op,  /* TODO: xadd */
  /*  0xc2 */ x86emuOp2_illegal_op,
  /*  0xc3 */ x86emuOp2_illegal_op,
  /*  0xc4 */ x86emuOp2_illegal_op,
  /*  0xc5 */ x86emuOp2_illegal_op,
  /*  0xc6 */ x86emuOp2_illegal_op,
  /*  0xc7 */ x86emuOp2_illegal_op,
  /*  0xc8 */ x86emuOp2_illegal_op,  /* TODO: bswap */
  /*  0xc9 */ x86emuOp2_illegal_op,  /* TODO: bswap */
  /*  0xca */ x86emuOp2_illegal_op,  /* TODO: bswap */
  /*  0xcb */ x86emuOp2_illegal_op,  /* TODO: bswap */
  /*  0xcc */ x86emuOp2_illegal_op,  /* TODO: bswap */
  /*  0xcd */ x86emuOp2_illegal_op,  /* TODO: bswap */
  /*  0xce */ x86emuOp2_illegal_op,  /* TODO: bswap */
  /*  0xcf */ x86emuOp2_illegal_op,  /* TODO: bswap */

  /*  0xd0 */ x86emuOp2_illegal_op,
  /*  0xd1 */ x86emuOp2_illegal_op,
  /*  0xd2 */ x86emuOp2_illegal_op,
  /*  0xd3 */ x86emuOp2_illegal_op,
  /*  0xd4 */ x86emuOp2_illegal_op,
  /*  0xd5 */ x86emuOp2_illegal_op,
  /*  0xd6 */ x86emuOp2_illegal_op,
  /*  0xd7 */ x86emuOp2_illegal_op,
  /*  0xd8 */ x86emuOp2_illegal_op,
  /*  0xd9 */ x86emuOp2_illegal_op,
  /*  0xda */ x86emuOp2_illegal_op,
  /*  0xdb */ x86emuOp2_illegal_op,
  /*  0xdc */ x86emuOp2_illegal_op,
  /*  0xdd */ x86emuOp2_illegal_op,
  /*  0xde */ x86emuOp2_illegal_op,
  /*  0xdf */ x86emuOp2_illegal_op,

  /*  0xe0 */ x86emuOp2_illegal_op,
  /*  0xe1 */ x86emuOp2_illegal_op,
  /*  0xe2 */ x86emuOp2_illegal_op,
  /*  0xe3 */ x86emuOp2_illegal_op,
  /*  0xe4 */ x86emuOp2_illegal_op,
  /*  0xe5 */ x86emuOp2_illegal_op,
  /*  0xe6 */ x86emuOp2_illegal_op,
  /*  0xe7 */ x86emuOp2_illegal_op,
  /*  0xe8 */ x86emuOp2_illegal_op,
  /*  0xe9 */ x86emuOp2_illegal_op,
  /*  0xea */ x86emuOp2_illegal_op,
  /*  0xeb */ x86emuOp2_illegal_op,
  /*  0xec */ x86emuOp2_illegal_op,
  /*  0xed */ x86emuOp2_illegal_op,
  /*  0xee */ x86emuOp2_illegal_op,
  /*  0xef */ x86emuOp2_illegal_op,

  /*  0xf0 */ x86emuOp2_illegal_op,
  /*  0xf1 */ x86emuOp2_illegal_op,
  /*  0xf2 */ x86emuOp2_illegal_op,
  /*  0xf3 */ x86emuOp2_illegal_op,
  /*  0xf4 */ x86emuOp2_illegal_op,
  /*  0xf5 */ x86emuOp2_illegal_op,
  /*  0xf6 */ x86emuOp2_illegal_op,
  /*  0xf7 */ x86emuOp2_illegal_op,
  /*  0xf8 */ x86emuOp2_illegal_op,
  /*  0xf9 */ x86emuOp2_illegal_op,
  /*  0xfa */ x86emuOp2_illegal_op,
  /*  0xfb */ x86emuOp2_illegal_op,
  /*  0xfc */ x86emuOp2_illegal_op,
  /*  0xfd */ x86emuOp2_illegal_op,
  /*  0xfe */ x86emuOp2_illegal_op,
  /*  0xff */ x86emuOp2_illegal_op,
};

