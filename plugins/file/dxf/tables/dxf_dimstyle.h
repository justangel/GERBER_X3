/*******************************************************************************
 * Author    :  Damir Bakiev                                                    *
 * Version   :  na                                                              *
 * Date      :  11 November 2021                                                *
 * Website   :  na                                                              *
 * Copyright :  Damir Bakiev 2016-2022                                          *
 * License:                                                                     * * Use, modification & distribution is subject to Boost Software License Ver 1. *
 * http://www.boost.org/LICENSE_1_0.txt                                         *
 *******************************************************************************/
#pragma once

#include "dxf_tableitem.h"

namespace Dxf {

struct DimStyle final : TableItem {
public:
    DimStyle(SectionParser* sp);

    // TableItem interface
public:
    void parse(CodeData& code) override;
    Type type() const override { return TableItem::DIMSTYLE; }
};

} // namespace Dxf