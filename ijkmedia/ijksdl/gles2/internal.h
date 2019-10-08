/*
 * Copyright (c) 2016 Bilibili
 * copyright (c) 2016 Zhang Rui <bbcallen@gmail.com>
 *
 * This file is part of ijkPlayer.
 *
 * ijkPlayer is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * ijkPlayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with ijkPlayer; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef IJKSDL__IJKSDL_GLES2__INTERNAL__H
#define IJKSDL__IJKSDL_GLES2__INTERNAL__H

#include <assert.h>
#include <stdlib.h>
#include "ijksdl/ijksdl_fourcc.h"
#include "ijksdl/ijksdl_log.h"
#include "ijksdl/ijksdl_gles2.h"
#include "ijksdl/ijksdl_vout.h"
#include "ijksdl/ijksdl_mutex.h"
#include "ijksdl/ijksdl_thread.h"

#define IJK_GLES_STRINGIZE(x)   #x
#define IJK_GLES_STRINGIZE2(x)  IJK_GLES_STRINGIZE(x)
#define IJK_GLES_STRING(x)      IJK_GLES_STRINGIZE2(x)

typedef struct IJK_GLES2_Renderer_Opaque IJK_GLES2_Renderer_Opaque;

#define MAX_SAMPLERS 6
#if IJK_GLES2_MAX_PLANE > MAX_SAMPLERS
#error invalid sampler count
#endif
typedef struct IJK_GLES2_ShaderProgram
{
    GLuint program;

    GLuint vertex_shader;
    GLuint fragment_shader;

    GLuint av4_position;
    GLuint av2_texcoord;
    GLuint um4_mvp;

    GLuint screenSize;
    GLuint us2_sampler[MAX_SAMPLERS];
    GLuint um3_color_conversion;

} IJK_GLES2_ShaderProgram;

typedef struct IJK_GLES2_Renderer
{
    IJK_GLES2_Renderer_Opaque *opaque;

    IJK_GLES2_ShaderProgram prog_frame_decode;
    IJK_GLES2_ShaderProgram prog_logo_detection;
    IJK_GLES2_ShaderProgram prog_logo_removal;
    IJK_GLES2_ShaderProgram prog_blit;
    IJK_GLES2_ShaderProgram prog_flip_blit;
    IJK_GLES2_ShaderProgram prog_logo_debug;

    GLuint plane_textures[IJK_GLES2_MAX_PLANE];

    GLuint logo_buffer;
    GLuint frame_current, framebuffer_decode;
    GLuint frame_last, framebuffer_lastFrame;
    GLuint frame_final, framebuffer_final;
    GLuint frame_reference_using, framebuffer_ref_using;
    GLuint frame_logo_using, framebuffer_logo_using;
    GLuint frame_reference_building, framebuffer_ref_building;
    GLuint frame_logo_building, framebuffer_logo_building;
    GLuint frame_logo_temp, framebuffer_logo_temp;

    /* logo detection */
    SDL_mutex * logo_data_mutex;
    SDL_cond *  logo_data_cond;
    SDL_Thread * logo_data_analyzer_tid;
    SDL_Thread _logo_data_analyzer_tid;
    char * logo_data;
    int abort_request;
    int logo_processing;

    GLboolean (*func_use)(IJK_GLES2_Renderer *renderer, IJK_GLES2_ShaderProgram * prog);
    GLsizei   (*func_getBufferWidth)(IJK_GLES2_Renderer *renderer, SDL_VoutOverlay *overlay);
    GLboolean (*func_uploadTexture)(IJK_GLES2_Renderer *renderer, SDL_VoutOverlay *overlay);
    GLvoid    (*func_destroy)(IJK_GLES2_Renderer *renderer);

    GLsizei buffer_width;
    GLsizei visible_width;

    GLfloat texcoords[8];

    GLfloat vertices[8];
    int     vertices_changed;

    int     format;
    int     gravity;
    GLsizei layer_width;
    GLsizei layer_height;
    int     frame_width;
    int     frame_height;
    int     frame_sar_num;
    int     frame_sar_den;

    int64_t current_milli;
    int64_t ref_using_milli;
    int64_t logo_built_milli;

    GLsizei last_buffer_width;
} IJK_GLES2_Renderer;

typedef struct IJK_GLES_Matrix
{
    GLfloat m[16];
} IJK_GLES_Matrix;
void IJK_GLES2_loadOrtho(IJK_GLES_Matrix *matrix, GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near, GLfloat far);

const char *IJK_GLES2_getVertexShader_default();
const char *IJK_GLES2_getFragmentShader_yuv420p();
const char *IJK_GLES2_getFragmentShader_yuv444p10le();
const char *IJK_GLES2_getFragmentShader_yuv420sp();
const char *IJK_GLES2_getFragmentShader_rgb();
const char *IJK_GLES2_getFragmentShader_flip_blit();
const char *IJK_GLES2_getFragmentShader_logo_detection();
const char *IJK_GLES2_getFragmentShader_logo_removal();
const char *IJK_GLES2_getFragmentShader_logo_debug();

const GLfloat *IJK_GLES2_getColorMatrix_bt709();
const GLfloat *IJK_GLES2_getColorMatrix_bt601();

IJK_GLES2_Renderer *IJK_GLES2_Renderer_create_base(const char *fragment_shader_source);
IJK_GLES2_Renderer *IJK_GLES2_Renderer_create_yuv420p();
IJK_GLES2_Renderer *IJK_GLES2_Renderer_create_yuv444p10le();
IJK_GLES2_Renderer *IJK_GLES2_Renderer_create_yuv420sp();
IJK_GLES2_Renderer *IJK_GLES2_Renderer_create_yuv420sp_vtb(SDL_VoutOverlay *overlay);
IJK_GLES2_Renderer *IJK_GLES2_Renderer_create_rgb565();
IJK_GLES2_Renderer *IJK_GLES2_Renderer_create_rgb888();
IJK_GLES2_Renderer *IJK_GLES2_Renderer_create_rgbx8888();

#endif
