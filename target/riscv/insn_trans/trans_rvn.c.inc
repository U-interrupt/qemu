/*
 * RISC-V translation routines for the RISC-V N instructions.
 *
 * Copyright (c) 2016-2017 Sagar Karandikar, sagark@eecs.berkeley.edu
 * Copyright (c) 2018 Peer Adelt, peer.adelt@hni.uni-paderborn.de
 *                    Bastian Koppelmann, kbastian@mail.uni-paderborn.de
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2 or later, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

static bool trans_uret(DisasContext *ctx, arg_uret *a)
{
#ifndef CONFIG_USER_ONLY
    if (has_ext(ctx, RVN)) {
        gen_helper_uret(cpu_pc, cpu_env);
        tcg_gen_exit_tb(NULL, 0); /* no chaining */
        ctx->base.is_jmp = DISAS_NORETURN;
        return true;
    }
#endif
    return false;
}

static bool trans_uipi(DisasContext *ctx, arg_uipi *a)
{

#ifndef CONFIG_USER_ONLY
    if (has_ext(ctx, RVN)) {
        TCGv addr = temp_new(ctx);
        TCGv_i32 op = tcg_constant_i32(a->imm);
        gen_helper_uipi_addr(addr, cpu_env, op);
        switch (a->imm) {
            case UIPI_SEND:
                return true;

            case UIPI_READ:
                return true;
                
            case UIPI_WRITE:
                TCGv data = get_gpr(ctx, a->rs1, EXT_NONE);
                tcg_gen_qemu_st_tl(data, addr, ctx->mem_idx, MO_TEUQ);
                return true;

            case UIPI_ACTIVATE:
                return true;

            case UIPI_DEACTIVATE:
                return true;
        }
        return true;
    }
#endif
    return false;
}