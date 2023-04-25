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

static bool trans_uipi_send(DisasContext *ctx, arg_uipi_send *a)
{

#ifndef CONFIG_USER_ONLY
    if (has_ext(ctx, RVN)) {
        TCGv src = get_gpr(ctx, a->rs1, EXT_NONE);
        gen_helper_uipi_send(cpu_env, src);
        return true;
    }
#endif
    return false;
}

static bool trans_uipi_read(DisasContext *ctx, arg_uipi_read *a)
{

#ifndef CONFIG_USER_ONLY
    if (has_ext(ctx, RVN)) {
        TCGv data = temp_new(ctx);
        gen_helper_uipi_read(data, cpu_env);
        TCGv dest = dest_gpr(ctx, a->rd);
        tcg_gen_mov_tl(dest, data);
        return true;
    }
#endif
    return false;
}

static bool trans_uipi_write(DisasContext *ctx, arg_uipi_write *a)
{

#ifndef CONFIG_USER_ONLY
    if (has_ext(ctx, RVN)) {
        TCGv src = get_gpr(ctx, a->rs1, EXT_NONE);
        gen_helper_uipi_write(cpu_env, src);
        return true;
    }
#endif
    return false;
}

static bool trans_uipi_activate(DisasContext *ctx, arg_uipi_activate *a)
{

#ifndef CONFIG_USER_ONLY
    if (has_ext(ctx, RVN)) {
        gen_helper_uipi_activate(cpu_env);
        return true;
    }
#endif
    return false;
}

static bool trans_uipi_deactivate(DisasContext *ctx, arg_uipi_deactivate *a)
{

#ifndef CONFIG_USER_ONLY
    if (has_ext(ctx, RVN)) {
        gen_helper_uipi_deactivate(cpu_env);
        return true;
    }
#endif
    return false;
}