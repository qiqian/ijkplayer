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
    uniform   lowp  sampler2D us2_SamplerX; // ref
    uniform   lowp  sampler2D us2_SamplerY; // cur
    uniform   lowp  sampler2D us2_SamplerZ; // lastFrame
    uniform   lowp  sampler2D us2_SamplerW; // logo

    vec3 detect_logo(float threshold, vec3 ref, vec3 cur, vec3 exsiting_logo)
    {
        vec3 diff = step(abs(ref - cur), vec3(threshold, threshold, threshold));
        float d = min(1.0, diff.x + diff.y + diff.z);
        return min(vec3(d, d, d), exsiting_logo);
    }
    void main()
    {
        vec2 uv = vec2(vv2_Texcoord.x, 1.0 - vv2_Texcoord.y);
        vec3 ref = texture2D(us2_SamplerX, uv).rgb;
        vec3 cur = texture2D(us2_SamplerY, uv).rgb;
        vec3 lastFrame = texture2D(us2_SamplerZ, uv).rgb;
        vec3 logo = texture2D(us2_SamplerW, uv).rgb;

        float threshold;
        vec3 diff;
        float d;
        vec3 merged_diff = logo;

        merged_diff = detect_logo(0.15, lastFrame, cur, merged_diff);
        merged_diff = detect_logo(0.3, ref, cur, merged_diff);

        gl_FragColor.xyz = merged_diff;
        gl_FragColor.w = 1.0;

        // min diff
//        gl_FragColor.xyz = max(abs(ref - cur), logo);
//        gl_FragColor.xyz = abs(ref - cur);


    }
);

const char *IJK_GLES2_getFragmentShader_logo_detection()
{
    return g_shader;
}
