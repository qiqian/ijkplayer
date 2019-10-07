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
        uniform   lowp  sampler2D us2_SamplerX;
        uniform   lowp  sampler2D us2_SamplerY;
        uniform   lowp  sampler2D us2_SamplerZ;
        uniform   lowp  sampler2D us2_SamplerW;
        uniform   lowp  sampler2D us2_SamplerP;
        uniform   lowp  sampler2D us2_SamplerQ;

        void main()
        {
            vec3 color;
            vec2 uv;
            // curr  ref-build  ref-use
            // final logo-build  logo-use
            if (vv2_Texcoord.x < 1.0 / 3.0)
            {
                if (vv2_Texcoord.y < 0.5) {
                    // curr
                    uv = vec2(vv2_Texcoord.x, vv2_Texcoord.y - 0.5) * vec2(3.0, 2.0);
                    uv = vec2(uv.x, 1.0 - uv.y);
                    color = texture2D(us2_SamplerX, uv).rgb;
                }
                else {
                    // final
                    uv = vv2_Texcoord * vec2(3.0, 2.0);
                    uv = vec2(uv.x, 1.0 - uv.y);
                    color = texture2D(us2_SamplerW, uv).rgb;
                }
            }
            else
            {
                if (vv2_Texcoord.x < 2.0 / 3.0) {
                    if (vv2_Texcoord.y < 0.5) {
                        // ref-build
                        uv = vec2(vv2_Texcoord.x - 1.0 / 3.0, vv2_Texcoord.y - 0.5) * vec2(3.0, 2.0);
                        uv = vec2(uv.x, 1.0 - uv.y);
                        color = texture2D(us2_SamplerY, uv).rgb;
                    }
                    else {
                        // logo-build
                        uv = vec2(vv2_Texcoord.x - 1.0 / 3.0, vv2_Texcoord.y) * vec2(3.0, 2.0);
                        uv = vec2(uv.x, 1.0 - uv.y);
                        color = texture2D(us2_SamplerP, uv).rgb;
                    }
                }
                else {
                    if (vv2_Texcoord.y < 0.5) {
                        // ref-use
                        uv = vec2(vv2_Texcoord.x - 2.0 / 3.0, vv2_Texcoord.y - 0.5) * vec2(3.0, 2.0);
                        uv = vec2(uv.x, 1.0 - uv.y);
                        color = texture2D(us2_SamplerZ, uv).rgb;
                    }
                    else {
                        // logo-use
                        uv = vec2(vv2_Texcoord.x - 2.0 / 3.0, vv2_Texcoord.y) * vec2(3.0, 2.0);
                        uv = vec2(uv.x, 1.0 - uv.y);
                        color = texture2D(us2_SamplerQ, uv).rgb;
                    }
                }
            }

            gl_FragColor.xyz = color;
            gl_FragColor.w = 1.0;
        }
);

const char *IJK_GLES2_getFragmentShader_logo_debug()
{
    return g_shader;
}
