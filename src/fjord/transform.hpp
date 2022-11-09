/*
 * Copyright (C) 2016-2022 Konstantin Polevik
 * All rights reserved
 *
 * This file is part of the FJORD. Redistribution and use in source and
 * binary forms, with or without modification, are permitted exclusively
 * under the terms of the MIT license. You should have received a copy of the
 * license with this file. If not, please visit:
 * https://github.com/out61h/fjord/blob/master/LICENSE
 */
#pragma once

#include "pixel.hpp"
#include "rect.hpp"
#include "symmetry.hpp"

namespace fjord
{
    struct Transform
    {
        Rect     geometry;
        Pixel    brightness;
        Pixel    contrast;
        Symmetry symmetry;
    };
} // namespace fjord
