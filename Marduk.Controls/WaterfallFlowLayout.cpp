#include "pch.h"
#include "WaterfallFlowLayout.h"

using namespace Marduk::Controls;

WaterfallFlowLayout::WaterfallFlowLayout(double spacing, double width, int stackCount)
{
    _spacing = spacing;
    _width = width;
    _units = new std::vector<WaterfallFlowUnit^>();
    _stacks = new std::vector<double>();

    for (int i = 0; i < stackCount; i++)
    {
        _stacks->push_back(0);
    }
}

WaterfallFlowLayout::~WaterfallFlowLayout()
{
    delete(_units);
    delete(_stacks);
}

void WaterfallFlowLayout::AddItem(int index, Platform::Object^ item, Size size)
{
    size.Width = (Width - ((StackCount - 1) * Spacing)) / StackCount;
    auto unit = ref new WaterfallFlowUnit(item, size);

    if (index != -1 && index < _units->size())
    {
        _units->insert(_units->begin() + index, unit);
        SetRelayoutIndex(index);
    }
    else
    {
        if (!(_requestRelayoutIndex < 0 || _requestRelayoutIndex >= _units->size()))
        {
            Relayout();
        }

        int minStackIndex = std::distance(_stacks->begin(), std::min_element(_stacks->begin(), _stacks->end()));
        unit->StackIndex = minStackIndex;

        if ((*_stacks)[minStackIndex] == 0)
        {
            unit->Offset = (*_stacks)[minStackIndex];
            (*_stacks)[minStackIndex] += size.Height;
        }
        else
        {
            unit->Offset = (*_stacks)[minStackIndex] + Spacing;
            (*_stacks)[minStackIndex] += size.Height + Spacing;
        }

        _units->push_back(unit);
    }
}

Platform::IntPtr WaterfallFlowLayout::GetVisableItems(VisualWindow window, int* firstIndex, int * lastIndex)
{
    if (!(_requestRelayoutIndex < 0 || _requestRelayoutIndex >= _units->size()))
    {
        Relayout();
    }

    std::vector<Platform::Object^>* result = new std::vector<Platform::Object^>();

    if (_units->size() == 0)
    {
        *firstIndex = -1;
        *lastIndex = -1;
        return result;
    }

    if (*firstIndex < 0)
    {
        for (int i = 0; i < _units->size(); i++)
        {
            if (_units->at(i)->Offset >= window.Offset)
            {
                *firstIndex = i - 1;
                break;
            }
        }
    }
    else
    {
        if (_units->at(0)->Offset + _units->at(0)->DesiredSize.Height > window.Offset)
        {
            *firstIndex = 0;
        }
        else
        {
            if (_units->at(*firstIndex)->Offset > window.Offset)
            {
                for (int i = *firstIndex; i >= 0; i--)
                {
                    if (_units->at(i)->Offset + _units->at(i)->DesiredSize.Height < window.Offset)
                    {
                        *firstIndex = i + 1;
                        break;
                    }
                }
            }
            else
            {
                for (int i = *firstIndex; i < _units->size(); i++)
                {
                    if (_units->at(i)->Offset + _units->at(i)->DesiredSize.Height >= window.Offset)
                    {
                        *firstIndex = i;
                        break;
                    }
                }
            }
        }
    }

    if (*firstIndex < 0)
    {
        *firstIndex = 0;
    }

    if (*lastIndex < 0)
    {
        for (int i = *firstIndex; i < _units->size(); i++)
        {
            if (_units->at(i)->Offset >= VisualWindowExtension::GetEndOffset(window))
            {
                *lastIndex = i - 1;
                break;
            }
        }
    }
    else
    {
        if (_units->at(_units->size() - 1)->Offset < VisualWindowExtension::GetEndOffset(window))
        {
            *lastIndex = _units->size() - 1;
        }
        else
        {
            if (_units->at(*lastIndex)->Offset > VisualWindowExtension::GetEndOffset(window))
            {
                for (int i = *lastIndex; i >= 0; i--)
                {
                    if (_units->at(i)->Offset < VisualWindowExtension::GetEndOffset(window))
                    {
                        *lastIndex = i;
                        break;
                    }
                }
            }
            else
            {
                for (int i = *lastIndex; i < _units->size(); i++)
                {
                    if (_units->at(i)->Offset >= VisualWindowExtension::GetEndOffset(window))
                    {
                        *lastIndex = i - 1;
                        break;
                    }
                }
            }
        }
    }

    if (*lastIndex < 0)
    {
        *lastIndex = _units->size() - 1;
    }

    for (int i = *firstIndex; i <= *lastIndex; i++)
    {
        result->push_back(_units->at(i)->Item);
    }

    return result;
}

Rect WaterfallFlowLayout::GetItemLayoutRect(int index)
{
    if (!(_requestRelayoutIndex < 0 || _requestRelayoutIndex >= _units->size()))
    {
        Relayout();
    }

    Rect result = Rect();
    double unitWidth = (Width - Spacing) / _stacks->size();

    auto unit = _units->at(index);

    result.Height = unit->DesiredSize.Height;
    result.Width = unit->DesiredSize.Width;
    result.X = unit->StackIndex * (unitWidth + Spacing);
    result.Y = unit->Offset;

    return result;
}

bool WaterfallFlowLayout::FillWindow(VisualWindow window)
{
    return *std::min_element(_stacks->begin(), _stacks->end()) >= VisualWindowExtension::GetEndOffset(window);
}

void WaterfallFlowLayout::ChangeItem(int index, Platform::Object^ item, Size size)
{
    if (item != nullptr)
    {
        _units->at(index)->Item = item;
    }

    if (size.Height != _units->at(index)->DesiredSize.Height)
    {
        _units->at(index)->DesiredSize = Size(_units->at(index)->DesiredSize.Width, size.Height);
        SetRelayoutIndex(index);
    }
}

void WaterfallFlowLayout::ChangePanelSize(double width)
{
    if (width != _width)
    {
        _width = width;
        SetRelayoutIndex(0);
    }
}

void WaterfallFlowLayout::ChangeStackCount(int stackCount)
{
    if (stackCount != _stacks->size())
    {
        _stacks->clear();

        for (int i = 0; i < stackCount; i++)
        {
            _stacks->push_back(0);
        }

        SetRelayoutIndex(0);
    }
}

void WaterfallFlowLayout::ChangeSpacing(double spacing)
{
    if (spacing != _spacing)
    {
        _spacing = spacing;
        SetRelayoutIndex(0);
    }
}

void WaterfallFlowLayout::RemoveItem(int index)
{
    SetRelayoutIndex(index);
    _units->erase(_units->begin() + index);
}

void WaterfallFlowLayout::Relayout()
{
    std::vector<bool>* flags = new std::vector<bool>();
    int stackCount = 0;

    for (int i = 0; i < _stacks->size(); i++)
    {
        (*_stacks)[i] = 0;
        flags->push_back(false);
    }


    for (int i = _requestRelayoutIndex - 1; i >= 0; i--)
    {
        auto unit = _units->at(i);

        if (!(*flags)[unit->StackIndex])
        {
            stackCount++;
            (*flags)[unit->StackIndex] = true;
            (*_stacks)[unit->StackIndex] = unit->Offset + unit->DesiredSize.Height;
        }

        if (stackCount == _stacks->size())
        {
            break;
        }
    }

    for (int i = _requestRelayoutIndex; i < _units->size(); i++)
    {
        auto unit = _units->at(i);

        Size size = Size((Width - Spacing) / _stacks->size(), unit->DesiredSize.Height);
        unit->DesiredSize = size;

        int minStackIndex = std::distance(_stacks->begin(), std::min_element(_stacks->begin(), _stacks->end()));
        unit->StackIndex = minStackIndex;

        if ((*_stacks)[minStackIndex] == 0)
        {
            unit->Offset = (*_stacks)[minStackIndex];
            (*_stacks)[minStackIndex] += size.Height;
        }
        else
        {
            unit->Offset = (*_stacks)[minStackIndex] + Spacing;
            (*_stacks)[minStackIndex] += size.Height + Spacing;
        }
    }

    _requestRelayoutIndex = -1;
}


void WaterfallFlowLayout::SetRelayoutIndex(int index)
{
    if (index >= 0 && _requestRelayoutIndex >= 0)
    {
        _requestRelayoutIndex = min(_requestRelayoutIndex, index);
    }
    else
    {
        _requestRelayoutIndex = max(_requestRelayoutIndex, index);
    }
}

void WaterfallFlowLayout::RemoveAll()
{
    _units->clear();
    for (int i = 0; i < _stacks->size(); i++)
    {
        (*_stacks)[i] = 0;
    }
    SetRelayoutIndex(0);
}

Size WaterfallFlowLayout::GetItemSize(int index)
{
    return _units->at(index)->DesiredSize;
}