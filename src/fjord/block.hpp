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

#include "image.hpp"
#include "transform.hpp"

namespace fjord
{
    struct RangeBlock
    {
        Image     original_image;
        Image     bordered_image;
        Image     window_image;
        Transform transform;
    };

} // namespace fjord
