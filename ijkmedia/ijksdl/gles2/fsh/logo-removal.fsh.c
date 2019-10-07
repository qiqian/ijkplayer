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

#include "ijksdl/gles2/internal.h"

static const char g_shader[] = IJK_GLES_STRING(
    precision highp float;
    varying   highp vec2 vv2_Texcoord;
    uniform   lowp  sampler2D us2_SamplerX; // current
    uniform   lowp  sampler2D us2_SamplerY; // logo-mask
    uniform   highp vec2 screenSize;

    void main()
    {
        vec2 uv = vec2(vv2_Texcoord.x, 1.0 - vv2_Texcoord.y);

        vec3 cur = texture2D(us2_SamplerX, uv).rgb;
        float logo = texture2D(us2_SamplerY, uv).r;

        if (logo == 1.0) {
            // de-logo
            int n = 0;
            int step = 1;
            float x_step = 1.0 / screenSize.x;
            float y_step = 1.0 / screenSize.y;

            gl_FragColor.xyz = cur * 0.5;
        }
        else
        {
            gl_FragColor.xyz = cur;
        }

        gl_FragColor.w = 1.0;
    }
);

const char *IJK_GLES2_getFragmentShader_logo_removal()
{
    return g_shader;
}
