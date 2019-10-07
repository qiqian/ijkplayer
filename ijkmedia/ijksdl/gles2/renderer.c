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

#include "internal.h"

static void IJK_GLES2_printProgramInfo(GLuint program)
{
    if (!program)
        return;

    GLint info_len = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_len);
    if (!info_len) {
        ALOGE("[GLES2][Program] empty info\n");
        return;
    }

    char    buf_stack[32];
    char   *buf_heap = NULL;
    char   *buf      = buf_stack;
    GLsizei buf_len  = sizeof(buf_stack) - 1;
    if (info_len > sizeof(buf_stack)) {
        buf_heap = (char*) malloc(info_len + 1);
        if (buf_heap) {
            buf     = buf_heap;
            buf_len = info_len;
        }
    }

    glGetProgramInfoLog(program, buf_len, NULL, buf);
    ALOGE("[GLES2][Program] error %s\n", buf);

    if (buf_heap)
        free(buf_heap);
}

static void IJK_GLES2_Renderer_reset_ShaderProgram(IJK_GLES2_ShaderProgram * prog)
{
    if (prog->vertex_shader)
        glDeleteShader(prog->vertex_shader);
    if (prog->fragment_shader)
        glDeleteShader(prog->fragment_shader);
    if (prog->program)
        glDeleteProgram(prog->program);

    prog->vertex_shader   = 0;
    prog->fragment_shader = 0;
    prog->program         = 0;
}

void IJK_GLES2_Renderer_reset(IJK_GLES2_Renderer *renderer)
{
    if (!renderer)
        return;

    IJK_GLES2_Renderer_reset_ShaderProgram(&renderer->prog_frame_decode);
    IJK_GLES2_Renderer_reset_ShaderProgram(&renderer->prog_logo_detection);
    IJK_GLES2_Renderer_reset_ShaderProgram(&renderer->prog_logo_removal);
    IJK_GLES2_Renderer_reset_ShaderProgram(&renderer->prog_logo_debug);
    IJK_GLES2_Renderer_reset_ShaderProgram(&renderer->prog_blit);
    IJK_GLES2_Renderer_reset_ShaderProgram(&renderer->prog_flip_blit);

    for (int i = 0; i < IJK_GLES2_MAX_PLANE; ++i) {
        if (renderer->plane_textures[i]) {
            glDeleteTextures(1, &renderer->plane_textures[i]);
            renderer->plane_textures[i] = 0;
        }
    }

    glDeleteTextures(1, &renderer->frame_current);
    glDeleteTextures(1, &renderer->frame_last);
    glDeleteTextures(1, &renderer->frame_final);
    glDeleteTextures(1, &renderer->frame_reference_building);
    glDeleteTextures(1, &renderer->frame_reference_using);
    glDeleteTextures(1, &renderer->frame_logo_building);
    glDeleteTextures(1, &renderer->frame_logo_using);
    glDeleteTextures(1, &renderer->frame_logo_temp);

    glDeleteFramebuffers(1, &renderer->framebuffer_decode);
    glDeleteFramebuffers(1, &renderer->framebuffer_lastFrame);
    glDeleteFramebuffers(1, &renderer->framebuffer_final);
    glDeleteFramebuffers(1, &renderer->framebuffer_ref_building);
    glDeleteFramebuffers(1, &renderer->framebuffer_ref_using);
    glDeleteFramebuffers(1, &renderer->framebuffer_logo_building);
    glDeleteFramebuffers(1, &renderer->framebuffer_logo_using);
    glDeleteFramebuffers(1, &renderer->framebuffer_logo_temp);
}

static void IJK_GLES2_Renderer_setup_Framebuffers(GLuint * frame, GLuint * framebuffer, int mask, int width, int height)
{
    glGenTextures(1, frame);
    glBindTexture(GL_TEXTURE_2D, *frame);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0,
            mask ? GL_R32F: GL_RGBA, width, height, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, 0); IJK_GLES2_checkError_TRACE("glTexImage2D");
    glGenFramebuffers(1, framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, *framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *frame, 0); IJK_GLES2_checkError_TRACE("glFramebufferTexture2D");
}

void IJK_GLES2_Renderer_setPostproess(IJK_GLES2_Renderer *renderer, int overlay_width, int overlay_height)
{
    // create textures
    IJK_GLES2_Renderer_setup_Framebuffers(&renderer->frame_current, &renderer->framebuffer_decode, 0, overlay_width, overlay_height);
    IJK_GLES2_Renderer_setup_Framebuffers(&renderer->frame_last, &renderer->framebuffer_lastFrame, 0, overlay_width, overlay_height);
    IJK_GLES2_Renderer_setup_Framebuffers(&renderer->frame_final, &renderer->framebuffer_final, 0, overlay_width, overlay_height);
    IJK_GLES2_Renderer_setup_Framebuffers(&renderer->frame_reference_using, &renderer->framebuffer_ref_using, 0, overlay_width, overlay_height);
    IJK_GLES2_Renderer_setup_Framebuffers(&renderer->frame_reference_building, &renderer->framebuffer_ref_building, 0, overlay_width, overlay_height);
    IJK_GLES2_Renderer_setup_Framebuffers(&renderer->frame_logo_building, &renderer->framebuffer_logo_building, 0, overlay_width, overlay_height);
    IJK_GLES2_Renderer_setup_Framebuffers(&renderer->frame_logo_using, &renderer->framebuffer_logo_using, 0, overlay_width, overlay_height);
    IJK_GLES2_Renderer_setup_Framebuffers(&renderer->frame_logo_temp, &renderer->framebuffer_logo_temp, 0, overlay_width, overlay_height);

    // reset time
    renderer->current_milli = 0;
    renderer->ref_using_milli = -1;
    renderer->logo_built_milli = 0;
}

void IJK_GLES2_Renderer_free(IJK_GLES2_Renderer *renderer)
{
    if (!renderer)
        return;

    if (renderer->func_destroy)
        renderer->func_destroy(renderer);

#if 0
    if (renderer->vertex_shader)    ALOGW("[GLES2] renderer: vertex_shader not deleted.\n");
    if (renderer->fragment_shader)  ALOGW("[GLES2] renderer: fragment_shader not deleted.\n");
    if (renderer->program)          ALOGW("[GLES2] renderer: program not deleted.\n");

    for (int i = 0; i < IJK_GLES2_MAX_PLANE; ++i) {
        if (renderer->plane_textures[i])
            ALOGW("[GLES2] renderer: plane texture[%d] not deleted.\n", i);
    }
#endif

    free(renderer);
}

void IJK_GLES2_Renderer_freeP(IJK_GLES2_Renderer **renderer)
{
    if (!renderer || !*renderer)
        return;

    IJK_GLES2_Renderer_free(*renderer);
    *renderer = NULL;
}

static GLboolean IJK_GLES2_create_ShaderProgram(IJK_GLES2_ShaderProgram * prog, const char * fragSrc)
{
    prog->vertex_shader = IJK_GLES2_loadShader(GL_VERTEX_SHADER, IJK_GLES2_getVertexShader_default());
    if (!prog->vertex_shader)
        return GL_FALSE;

    prog->fragment_shader = IJK_GLES2_loadShader(GL_FRAGMENT_SHADER, fragSrc);
    if (!prog->fragment_shader)
        return GL_FALSE;

    prog->program = glCreateProgram();                          IJK_GLES2_checkError("glCreateProgram");
    if (!prog->program)
        return GL_FALSE;

    glAttachShader(prog->program, prog->vertex_shader);     IJK_GLES2_checkError("glAttachShader(vertex)");
    glAttachShader(prog->program, prog->fragment_shader);   IJK_GLES2_checkError("glAttachShader(fragment)");
    glLinkProgram(prog->program);                               IJK_GLES2_checkError("glLinkProgram");
    GLint link_status = GL_FALSE;
    glGetProgramiv(prog->program, GL_LINK_STATUS, &link_status);
    if (!link_status)
        return GL_FALSE;

    prog->av4_position = glGetAttribLocation(prog->program, "av4_Position");                IJK_GLES2_checkError_TRACE("glGetAttribLocation(av4_Position)");
    prog->av2_texcoord = glGetAttribLocation(prog->program, "av2_Texcoord");                IJK_GLES2_checkError_TRACE("glGetAttribLocation(av2_Texcoord)");
    prog->um4_mvp      = glGetUniformLocation(prog->program, "um4_ModelViewProjection");    IJK_GLES2_checkError_TRACE("glGetUniformLocation(um4_ModelViewProjection)");

    prog->screenSize   = glGetUniformLocation(prog->program, "screenSize");

    prog->us2_sampler[0] = glGetUniformLocation(prog->program, "us2_SamplerX"); IJK_GLES2_checkError_TRACE("glGetUniformLocation(us2_SamplerX)");
    prog->us2_sampler[1] = glGetUniformLocation(prog->program, "us2_SamplerY");
    prog->us2_sampler[2] = glGetUniformLocation(prog->program, "us2_SamplerZ");
    prog->us2_sampler[3] = glGetUniformLocation(prog->program, "us2_SamplerW");
    prog->us2_sampler[4] = glGetUniformLocation(prog->program, "us2_SamplerP");
    prog->us2_sampler[5] = glGetUniformLocation(prog->program, "us2_SamplerQ");

    prog->um3_color_conversion = glGetUniformLocation(prog->program, "um3_ColorConversion"); //IJK_GLES2_checkError_TRACE("glGetUniformLocation(um3_ColorConversionMatrix)");

    return GL_TRUE;
}

IJK_GLES2_Renderer *IJK_GLES2_Renderer_create_base(const char *fragment_shader_source)
{
    assert(fragment_shader_source);

    IJK_GLES2_Renderer *renderer = (IJK_GLES2_Renderer *)calloc(1, sizeof(IJK_GLES2_Renderer));
    if (!renderer)
        goto fail;

    if (!IJK_GLES2_create_ShaderProgram(&renderer->prog_frame_decode, fragment_shader_source))
        goto fail;

    return renderer;

fail:

    if (renderer && renderer->prog_frame_decode.program)
        IJK_GLES2_printProgramInfo(renderer->prog_frame_decode.program);

    IJK_GLES2_Renderer_free(renderer);
    return NULL;
}


IJK_GLES2_Renderer *IJK_GLES2_Renderer_create(SDL_VoutOverlay *overlay)
{
    if (!overlay)
        return NULL;

    IJK_GLES2_printString("Version", GL_VERSION);
    IJK_GLES2_printString("Vendor", GL_VENDOR);
    IJK_GLES2_printString("Renderer", GL_RENDERER);
    IJK_GLES2_printString("Extensions", GL_EXTENSIONS);

    IJK_GLES2_Renderer *renderer = NULL;
    switch (overlay->format) {
        case SDL_FCC_RV16:      renderer = IJK_GLES2_Renderer_create_rgb565(); break;
        case SDL_FCC_RV24:      renderer = IJK_GLES2_Renderer_create_rgb888(); break;
        case SDL_FCC_RV32:      renderer = IJK_GLES2_Renderer_create_rgbx8888(); break;
#ifdef __APPLE__
        case SDL_FCC_NV12:      renderer = IJK_GLES2_Renderer_create_yuv420sp(); break;
        case SDL_FCC__VTB:      renderer = IJK_GLES2_Renderer_create_yuv420sp_vtb(overlay); break;
#endif
        case SDL_FCC_YV12:      renderer = IJK_GLES2_Renderer_create_yuv420p(); break;
        case SDL_FCC_I420:      renderer = IJK_GLES2_Renderer_create_yuv420p(); break;
        case SDL_FCC_I444P10LE: renderer = IJK_GLES2_Renderer_create_yuv444p10le(); break;
        default:
            ALOGE("[GLES2] unknown format %4s(%d)\n", (char *)&overlay->format, overlay->format);
            return NULL;
    }

    renderer->format = overlay->format;

    // post process
    if (!IJK_GLES2_create_ShaderProgram(&renderer->prog_logo_detection, IJK_GLES2_getFragmentShader_logo_detection()))
        goto fail;
    if (!IJK_GLES2_create_ShaderProgram(&renderer->prog_logo_removal, IJK_GLES2_getFragmentShader_logo_removal()))
        goto fail;
    if (!IJK_GLES2_create_ShaderProgram(&renderer->prog_logo_debug, IJK_GLES2_getFragmentShader_logo_debug()))
        goto fail;
    if (!IJK_GLES2_create_ShaderProgram(&renderer->prog_blit, IJK_GLES2_getFragmentShader_rgb()))
        goto fail;
    if (!IJK_GLES2_create_ShaderProgram(&renderer->prog_flip_blit, IJK_GLES2_getFragmentShader_flip_blit()))
        goto fail;

    return renderer;

    fail:

    if (renderer && renderer->prog_frame_decode.program)
        IJK_GLES2_printProgramInfo(renderer->prog_frame_decode.program);
    if (renderer && renderer->prog_logo_detection.program)
        IJK_GLES2_printProgramInfo(renderer->prog_logo_detection.program);
    if (renderer && renderer->prog_logo_removal.program)
        IJK_GLES2_printProgramInfo(renderer->prog_logo_removal.program);
    if (renderer && renderer->prog_logo_debug.program)
        IJK_GLES2_printProgramInfo(renderer->prog_logo_debug.program);
    if (renderer && renderer->prog_blit.program)
        IJK_GLES2_printProgramInfo(renderer->prog_blit.program);
    if (renderer && renderer->prog_flip_blit.program)
        IJK_GLES2_printProgramInfo(renderer->prog_flip_blit.program);

    IJK_GLES2_Renderer_free(renderer);
    return NULL;
}

GLboolean IJK_GLES2_Renderer_isValid(IJK_GLES2_Renderer *renderer)
{
    return renderer && renderer->prog_frame_decode.program && renderer->prog_logo_detection.program && renderer->prog_logo_removal.program
            ? GL_TRUE : GL_FALSE;
}

GLboolean IJK_GLES2_Renderer_isFormat(IJK_GLES2_Renderer *renderer, int format)
{
    if (!IJK_GLES2_Renderer_isValid(renderer))
        return GL_FALSE;

    return renderer->format == format ? GL_TRUE : GL_FALSE;
}

/*
 * Per-Context routine
 */
GLboolean IJK_GLES2_Renderer_setupGLES()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);       IJK_GLES2_checkError_TRACE("glClearColor");
    glEnable(GL_CULL_FACE);                     IJK_GLES2_checkError_TRACE("glEnable(GL_CULL_FACE)");
    glCullFace(GL_BACK);                        IJK_GLES2_checkError_TRACE("glCullFace");
    glDisable(GL_DEPTH_TEST);

    char * version = (char *)glGetString(GL_VERSION);
    ALOGI("[QQ] GL version : %s", version);
    return GL_TRUE;
}

static void IJK_GLES2_Renderer_Vertices_reset(IJK_GLES2_Renderer *renderer)
{
    renderer->vertices[0] = -1.0f;
    renderer->vertices[1] = -1.0f;
    renderer->vertices[2] =  1.0f;
    renderer->vertices[3] = -1.0f;
    renderer->vertices[4] = -1.0f;
    renderer->vertices[5] =  1.0f;
    renderer->vertices[6] =  1.0f;
    renderer->vertices[7] =  1.0f;
}

static void IJK_GLES2_Renderer_Vertices_apply(IJK_GLES2_Renderer *renderer)
{
    switch (renderer->gravity) {
        case IJK_GLES2_GRAVITY_RESIZE_ASPECT:
            break;
        case IJK_GLES2_GRAVITY_RESIZE_ASPECT_FILL:
            break;
        case IJK_GLES2_GRAVITY_RESIZE:
            IJK_GLES2_Renderer_Vertices_reset(renderer);
            return;
        default:
            ALOGE("[GLES2] unknown gravity %d\n", renderer->gravity);
            IJK_GLES2_Renderer_Vertices_reset(renderer);
            return;
    }

    if (renderer->layer_width <= 0 ||
        renderer->layer_height <= 0 ||
        renderer->frame_width <= 0 ||
        renderer->frame_height <= 0)
    {
        ALOGE("[GLES2] invalid width/height for gravity aspect\n");
        IJK_GLES2_Renderer_Vertices_reset(renderer);
        return;
    }

    float width     = renderer->frame_width;
    float height    = renderer->frame_height;

    if (renderer->frame_sar_num > 0 && renderer->frame_sar_den > 0) {
        width = width * renderer->frame_sar_num / renderer->frame_sar_den;
    }

    const float dW  = (float)renderer->layer_width	/ width;
    const float dH  = (float)renderer->layer_height / height;
    float dd        = 1.0f;
    float nW        = 1.0f;
    float nH        = 1.0f;

    switch (renderer->gravity) {
        case IJK_GLES2_GRAVITY_RESIZE_ASPECT_FILL:  dd = FFMAX(dW, dH); break;
        case IJK_GLES2_GRAVITY_RESIZE_ASPECT:       dd = FFMIN(dW, dH); break;
    }

    nW = (width  * dd / (float)renderer->layer_width);
    nH = (height * dd / (float)renderer->layer_height);

    renderer->vertices[0] = - nW;
    renderer->vertices[1] = - nH;
    renderer->vertices[2] =   nW;
    renderer->vertices[3] = - nH;
    renderer->vertices[4] = - nW;
    renderer->vertices[5] =   nH;
    renderer->vertices[6] =   nW;
    renderer->vertices[7] =   nH;
}

static void IJK_GLES2_Renderer_Vertices_reloadVertex(IJK_GLES2_Renderer *renderer, IJK_GLES2_ShaderProgram * prog)
{
    glVertexAttribPointer(prog->av4_position, 2, GL_FLOAT, GL_FALSE, 0, renderer->vertices);    IJK_GLES2_checkError_TRACE("glVertexAttribPointer(av2_texcoord)");
    glEnableVertexAttribArray(prog->av4_position);                                      IJK_GLES2_checkError_TRACE("glEnableVertexAttribArray(av2_texcoord)");
}

#define IJK_GLES2_GRAVITY_MIN                   (0)
#define IJK_GLES2_GRAVITY_RESIZE                (0) // Stretch to fill layer bounds.
#define IJK_GLES2_GRAVITY_RESIZE_ASPECT         (1) // Preserve aspect ratio; fit within layer bounds.
#define IJK_GLES2_GRAVITY_RESIZE_ASPECT_FILL    (2) // Preserve aspect ratio; fill layer bounds.
#define IJK_GLES2_GRAVITY_MAX                   (2)

GLboolean IJK_GLES2_Renderer_setGravity(IJK_GLES2_Renderer *renderer, int gravity, GLsizei layer_width, GLsizei layer_height)
{
    if (renderer->gravity != gravity && gravity >= IJK_GLES2_GRAVITY_MIN && gravity <= IJK_GLES2_GRAVITY_MAX)
        renderer->vertices_changed = 1;
    else if (renderer->layer_width != layer_width)
        renderer->vertices_changed = 1;
    else if (renderer->layer_height != layer_height)
        renderer->vertices_changed = 1;
    else
        return GL_TRUE;

    renderer->gravity      = gravity;
    renderer->layer_width  = layer_width;
    renderer->layer_height = layer_height;
    return GL_TRUE;
}

static void IJK_GLES2_Renderer_TexCoords_reset(IJK_GLES2_Renderer *renderer)
{
    renderer->texcoords[0] = 0.0f;
    renderer->texcoords[1] = 1.0f;
    renderer->texcoords[2] = 1.0f;
    renderer->texcoords[3] = 1.0f;
    renderer->texcoords[4] = 0.0f;
    renderer->texcoords[5] = 0.0f;
    renderer->texcoords[6] = 1.0f;
    renderer->texcoords[7] = 0.0f;
}

static void IJK_GLES2_Renderer_TexCoords_cropRight(IJK_GLES2_Renderer *renderer, GLfloat cropRight)
{
    ALOGE("IJK_GLES2_Renderer_TexCoords_cropRight\n");
    renderer->texcoords[0] = 0.0f;
    renderer->texcoords[1] = 1.0f;
    renderer->texcoords[2] = 1.0f - cropRight;
    renderer->texcoords[3] = 1.0f;
    renderer->texcoords[4] = 0.0f;
    renderer->texcoords[5] = 0.0f;
    renderer->texcoords[6] = 1.0f - cropRight;
    renderer->texcoords[7] = 0.0f;
}

static void IJK_GLES2_Renderer_TexCoords_reloadVertex(IJK_GLES2_Renderer *renderer, IJK_GLES2_ShaderProgram * prog)
{
    glVertexAttribPointer(prog->av2_texcoord, 2, GL_FLOAT, GL_FALSE, 0, renderer->texcoords);   IJK_GLES2_checkError_TRACE("glVertexAttribPointer(av2_texcoord)");
    glEnableVertexAttribArray(prog->av2_texcoord);                                              IJK_GLES2_checkError_TRACE("glEnableVertexAttribArray(av2_texcoord)");
}

/*
 * Per-Renderer routine
 */
static void IJK_GLES2_Renderer_setup_vertexAttrib(IJK_GLES2_Renderer *renderer, IJK_GLES2_ShaderProgram *prog)
{
    glUseProgram(prog->program);            IJK_GLES2_checkError_TRACE("glUseProgram");

    IJK_GLES_Matrix modelViewProj;
    IJK_GLES2_loadOrtho(&modelViewProj, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(prog->um4_mvp, 1, GL_FALSE, modelViewProj.m);                    IJK_GLES2_checkError_TRACE("glUniformMatrix4fv(um4_mvp)");

    IJK_GLES2_Renderer_TexCoords_reset(renderer);
    IJK_GLES2_Renderer_TexCoords_reloadVertex(renderer, prog);

    IJK_GLES2_Renderer_Vertices_reset(renderer);
    IJK_GLES2_Renderer_Vertices_reloadVertex(renderer, prog);
}
static void IJK_GLES2_Renderer_setupTexture(IJK_GLES2_ShaderProgram *prog, int index, GLuint tex)
{
    glActiveTexture(GL_TEXTURE0 + index);
    glBindTexture(GL_TEXTURE_2D, tex);
    glUniform1i(prog->us2_sampler[index], index);
}
GLboolean IJK_GLES2_Renderer_use(IJK_GLES2_Renderer *renderer, IJK_GLES2_ShaderProgram *prog)
{
    if (!prog)
        return GL_FALSE;

    assert(renderer->func_use);
    glUseProgram(prog->program);            IJK_GLES2_checkError_TRACE("glUseProgram");
    if (!renderer->func_use(renderer, prog))
        return GL_FALSE;

    IJK_GLES2_Renderer_setup_vertexAttrib(renderer, prog);
    return GL_TRUE;
}

static void IJK_GLES2_blit(IJK_GLES2_Renderer *renderer, GLuint src, GLuint dstFB, int flip)
{
    IJK_GLES2_ShaderProgram * prog = flip ? &renderer->prog_flip_blit : &renderer->prog_blit;
    IJK_GLES2_Renderer_setup_vertexAttrib(renderer, prog);
    glBindFramebuffer(GL_FRAMEBUFFER, dstFB); IJK_GLES2_checkError_TRACE("glBindFramebuffer");
    IJK_GLES2_Renderer_setupTexture(prog, 0, src);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);      IJK_GLES2_checkError_TRACE("glDrawArrays");
}

/*
 * Per-Frame routine
 */
GLboolean IJK_GLES2_Renderer_renderOverlay(IJK_GLES2_Renderer *renderer, SDL_VoutOverlay *overlay)
{
    if (!renderer || !renderer->func_uploadTexture)
        return GL_FALSE;

    if (!IJK_GLES2_Renderer_use(renderer, &renderer->prog_frame_decode)) {
        ALOGD("[EGL] Could not use render.");
        return GL_FALSE;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, renderer->framebuffer_decode);
    glClear(GL_COLOR_BUFFER_BIT);               IJK_GLES2_checkError_TRACE("glClear");

    GLsizei visible_width  = renderer->frame_width;
    GLsizei visible_height = renderer->frame_height;
    if (overlay) {
        visible_width  = overlay->w;
        visible_height = overlay->h;
        if (renderer->frame_width   != visible_width    ||
            renderer->frame_height  != visible_height   ||
            renderer->frame_sar_num != overlay->sar_num ||
            renderer->frame_sar_den != overlay->sar_den) {

            renderer->frame_width   = visible_width;
            renderer->frame_height  = visible_height;
            renderer->frame_sar_num = overlay->sar_num;
            renderer->frame_sar_den = overlay->sar_den;

            renderer->vertices_changed = 1;
        }

        renderer->last_buffer_width = renderer->func_getBufferWidth(renderer, overlay);

        if (!renderer->func_uploadTexture(renderer, overlay))
            return GL_FALSE;
    } else {
        // NULL overlay means force reload vertice
        renderer->vertices_changed = 1;
    }

    GLsizei buffer_width = renderer->last_buffer_width;
    if (renderer->vertices_changed ||
        (buffer_width > 0 &&
         buffer_width > visible_width &&
         buffer_width != renderer->buffer_width &&
         visible_width != renderer->visible_width)){

        renderer->vertices_changed = 0;

        IJK_GLES2_Renderer_Vertices_apply(renderer);
        IJK_GLES2_Renderer_Vertices_reloadVertex(renderer, &renderer->prog_frame_decode);

        renderer->buffer_width  = buffer_width;
        renderer->visible_width = visible_width;

        GLsizei padding_pixels     = buffer_width - visible_width;
        GLfloat padding_normalized = ((GLfloat)padding_pixels) / buffer_width;

        IJK_GLES2_Renderer_TexCoords_reset(renderer);
        IJK_GLES2_Renderer_TexCoords_cropRight(renderer, padding_normalized);
        IJK_GLES2_Renderer_TexCoords_reloadVertex(renderer, &renderer->prog_frame_decode);
    }
    // decode
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);      IJK_GLES2_checkError_TRACE("glDrawArrays");

    // timestamp
    struct timeval  tv;
    gettimeofday(&tv, NULL);
    renderer->current_milli = ((int64_t)tv.tv_sec) * 1000 + ((int64_t)tv.tv_usec) / 1000;

    // update exsting logo template
    if (renderer->logo_built_milli > 0) {
        IJK_GLES2_blit(renderer, renderer->frame_logo_using, renderer->framebuffer_logo_temp, 1);
        IJK_GLES2_Renderer_setup_vertexAttrib(renderer, &renderer->prog_logo_detection);
        glBindFramebuffer(GL_FRAMEBUFFER, renderer->framebuffer_logo_using); IJK_GLES2_checkError_TRACE("glBindFramebuffer");
        IJK_GLES2_Renderer_setupTexture(&renderer->prog_logo_detection, 0, renderer->frame_reference_using);
        IJK_GLES2_Renderer_setupTexture(&renderer->prog_logo_detection, 1, renderer->frame_current);
        IJK_GLES2_Renderer_setupTexture(&renderer->prog_logo_detection, 2, renderer->frame_last);
        IJK_GLES2_Renderer_setupTexture(&renderer->prog_logo_detection, 3, renderer->frame_logo_temp);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);      IJK_GLES2_checkError_TRACE("glDrawArrays");
    }

    if (renderer->ref_using_milli < 0) {
        // discard 1st frame
        renderer->ref_using_milli = 0;
    }
    // need to start building a new logo ?
    else if (renderer->ref_using_milli == 0 || // init
            (renderer->logo_built_milli > 0 && renderer->current_milli - renderer->logo_built_milli > 1000 * 10 // logo too old
                && renderer->logo_built_milli > renderer->ref_using_milli)) { // not already building
        // update ref
        renderer->ref_using_milli = renderer->current_milli;
        IJK_GLES2_blit(renderer, renderer->frame_current, renderer->framebuffer_ref_building, 1);
        // clear logo
        glBindFramebuffer(GL_FRAMEBUFFER, renderer->framebuffer_logo_building); IJK_GLES2_checkError_TRACE("glBindFramebuffer");
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ALOGI("[QQ] start building new logo");
    }
    // build logo
    else if (renderer->ref_using_milli > renderer->logo_built_milli) {
        IJK_GLES2_blit(renderer, renderer->frame_logo_building, renderer->framebuffer_logo_temp, 1);
        IJK_GLES2_Renderer_setup_vertexAttrib(renderer, &renderer->prog_logo_detection);
        glBindFramebuffer(GL_FRAMEBUFFER, renderer->framebuffer_logo_building); IJK_GLES2_checkError_TRACE("glBindFramebuffer");
        IJK_GLES2_Renderer_setupTexture(&renderer->prog_logo_detection, 0, renderer->frame_reference_building);
        IJK_GLES2_Renderer_setupTexture(&renderer->prog_logo_detection, 1, renderer->frame_current);
        IJK_GLES2_Renderer_setupTexture(&renderer->prog_logo_detection, 2, renderer->frame_last);
        IJK_GLES2_Renderer_setupTexture(&renderer->prog_logo_detection, 3, renderer->frame_logo_temp);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);      IJK_GLES2_checkError_TRACE("glDrawArrays");
        // promote to new logo
        if (renderer->current_milli - renderer->ref_using_milli > 1000 * 120) {
            renderer->logo_built_milli = renderer->current_milli;
            IJK_GLES2_blit(renderer, renderer->frame_reference_building, renderer->framebuffer_ref_using, 1);
            IJK_GLES2_blit(renderer, renderer->frame_logo_building, renderer->framebuffer_logo_using, 1);
            ALOGI("[QQ] built new logo");
        }
    }

#if 1
    // display
    if (renderer->logo_built_milli > 0) {
        // de-logo
        IJK_GLES2_Renderer_setup_vertexAttrib(renderer, &renderer->prog_logo_removal);
        glBindFramebuffer(GL_FRAMEBUFFER, renderer->framebuffer_final); IJK_GLES2_checkError_TRACE("glBindFramebuffer"); // system
        IJK_GLES2_Renderer_setupTexture(&renderer->prog_logo_removal, 0, renderer->frame_current);
        IJK_GLES2_Renderer_setupTexture(&renderer->prog_logo_removal, 1, renderer->frame_logo_using);
        glUniform2f(renderer->prog_logo_removal.screenSize, renderer->frame_width, renderer->frame_height);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);      IJK_GLES2_checkError_TRACE("glDrawArrays");
    }
    else {
        // output original frame
        IJK_GLES2_blit(renderer, renderer->frame_current, renderer->framebuffer_final, 1);
    }
#endif

#if 1
    // debug
    IJK_GLES2_Renderer_setup_vertexAttrib(renderer, &renderer->prog_logo_debug);
    glBindFramebuffer(GL_FRAMEBUFFER, 0); IJK_GLES2_checkError_TRACE("glBindFramebuffer");

    // last  ref-build   ref-use
    IJK_GLES2_Renderer_setupTexture(&renderer->prog_logo_debug, 0, renderer->frame_current);
    IJK_GLES2_Renderer_setupTexture(&renderer->prog_logo_debug, 1, renderer->frame_reference_building);
    IJK_GLES2_Renderer_setupTexture(&renderer->prog_logo_debug, 2, renderer->frame_reference_using);

    // curr  logo-build  logo-use
    IJK_GLES2_Renderer_setupTexture(&renderer->prog_logo_debug, 3, renderer->frame_final);
    IJK_GLES2_Renderer_setupTexture(&renderer->prog_logo_debug, 4, renderer->frame_logo_building);
    IJK_GLES2_Renderer_setupTexture(&renderer->prog_logo_debug, 5, renderer->frame_logo_using);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);      IJK_GLES2_checkError_TRACE("glDrawArrays");

#endif

    // finish
    IJK_GLES2_blit(renderer, renderer->frame_current, renderer->framebuffer_lastFrame, 1);

    // swap
//    IJK_GLES2_blit(renderer, renderer->frame_final, 0, 1);

    return GL_TRUE;
}
