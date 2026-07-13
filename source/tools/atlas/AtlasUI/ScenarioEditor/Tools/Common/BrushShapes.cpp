/* Copyright (C) 2026 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "BrushShapes.h"

#include <cmath>
#include <wx/translation.h>

namespace
{

template<typename F>
std::vector<float> GenerateData(int size, F falloff)
{
	std::vector<float> data(size * size);
	int i = 0;
	const int mid = size - 1;
	for (int y = 0; y < size; ++y)
	{
		for (int x = 0; x < size; ++x)
		{
			const float dx = (2*x - mid) / static_cast<float>(size);
			const float dy = (2*y - mid) / static_cast<float>(size);
			data[i++] = falloff(dx, dy);
		}
	}
	return data;
}

class CircleBrushShape : public BrushShape
{
public:
	wxString GetName() const override
	{
		return _("Circle");
	}

	std::vector<float> GetData(int size) const override
	{
		return GenerateData(size, [](float dx, float dy) {
			const float distSq = dx*dx + dy*dy;
			if (distSq > 1.f)
				return 0.f;
			return (std::sqrt(2.f - distSq) - 1.f) / (std::sqrt(2.f) - 1.f);
		});
	}
};

class SquareBrushShape : public BrushShape
{
public:
	wxString GetName() const override
	{
		return _("Square");
	}

	std::vector<float> GetData(int size) const override
	{
		return GenerateData(size, [](float, float) {
			return 1.f;
		});
	}
};

class DiamondBrushShape : public BrushShape
{
public:
	wxString GetName() const override
	{
		return _("Diamond");
	}

	std::vector<float> GetData(int size) const override
	{
		return GenerateData(size, [](float dx, float dy) {
			return std::abs(dx) + std::abs(dy) <= 1.f ? 1.f : 0.f;
		});
	}
};

}

const std::vector<const BrushShape*>& BrushShape::GetShapes()
{
	static const CircleBrushShape circle;
	static const SquareBrushShape square;
	static const DiamondBrushShape diamond;
	static const std::vector<const BrushShape*> shapes{ &circle, &square, &diamond };
	return shapes;
}
