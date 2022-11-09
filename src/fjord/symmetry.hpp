/*
 * Copyright (C) 2016-2022 Konstantin Polevik
 * All rights reserved
 *
 * This file is part of the FJORD. Redistribution and use in source and
 * binary forms, with or without modification, are permitted exclusively
 * under the terms of the MIT license. You should have received a copy of the
 * license with this file. If not, please visit:
 * https://github.com/out61h/fjord/blob/main/LICENSE.
 */
#pragma once

namespace fjord
{
    enum class Symmetry
    {
        identity,
        rotate_90,
        rotate_180,
        rotate_270,
        reflection_m1,
        reflection_m2,
        reflection_m3,
        reflection_m4,
        count
    };
} // namespace ifs
