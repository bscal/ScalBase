#pragma once

#include "../Base.h"

struct Vec2i
{
	int x;
	int y;

	_FORCE_INLINE_ Vec2i Add(Vec2i o) const;
    _FORCE_INLINE_ Vec2i AddValue(int value) const;
    _FORCE_INLINE_ Vec2i Subtract(Vec2i o) const;
    _FORCE_INLINE_ Vec2i SubtractValue(int value) const;
    _FORCE_INLINE_ Vec2i Scale(int value) const;
    _FORCE_INLINE_ Vec2i Multiply(Vec2i o) const;
    _FORCE_INLINE_ Vec2i Divide(Vec2i o) const;
    _FORCE_INLINE_ float Distance(Vec2i o) const;
    _FORCE_INLINE_ float SqrDistance(Vec2i o) const;
    _FORCE_INLINE_ int ManhattanDistance(Vec2i o) const; // Cost 1 per cardinal move, 2 per diagonal 
    _FORCE_INLINE_ int ManhattanDistanceWithCosts(Vec2i o) const; // Cost 10 per cardinal, 14 per diagonal
    _FORCE_INLINE_ Vec2i Negate() const;
    _FORCE_INLINE_ Vec2i VecMin(Vec2i min) const;
    _FORCE_INLINE_ Vec2i VecMax(Vec2i max) const;
};

constexpr static Vec2i Vec2i_ONE     = { 1, 1 };
constexpr static Vec2i Vec2i_ZERO    = { 0, 0 };
constexpr static Vec2i Vec2i_MAX     = { INT32_MAX, INT32_MAX };
constexpr static Vec2i Vec2i_NULL    = Vec2i_MAX;

constexpr static Vec2i Vec2i_UP      = { 0, -1 };
constexpr static Vec2i Vec2i_DOWN    = { 0, 1 };
constexpr static Vec2i Vec2i_LEFT    = { -1, 0 };
constexpr static Vec2i Vec2i_RIGHT   = { 1, 0 };
constexpr static Vec2i Vec2i_NW      = { -1, 1 };
constexpr static Vec2i Vec2i_NE      = { 1, 1 };
constexpr static Vec2i Vec2i_SW      = { -1, -1 };
constexpr static Vec2i Vec2i_SE      = { 1, -1 };

constexpr static Vec2i Vec2i_CARDINALS[4] = { Vec2i_UP, Vec2i_RIGHT, Vec2i_DOWN, Vec2i_LEFT };
constexpr static Vec2i Vec2i_DIAGONALS[4] = { Vec2i_NW, Vec2i_NE, Vec2i_SW, Vec2i_SE };

constexpr static Vec2i Vec2i_NEIGHTBORS[8] = {
    Vec2i_NW,   Vec2i_UP ,  Vec2i_NE,
    Vec2i_LEFT,          Vec2i_RIGHT,
    Vec2i_SW,   Vec2i_DOWN, Vec2i_SE };

_FORCE_INLINE_ i64 Vec2iPackInt64(Vec2i v);
_FORCE_INLINE_ Vec2i Vec2iUnpackInt64(i64 packedI64);

i64 Vec2iPackInt64(Vec2i v)
{
    return ((i64)(v.x) << 32ll) | (i64)v.y;
}

Vec2i Vec2iUnpackInt64(i64 packedI64)
{
    Vec2i res;
    res.x = (int)(packedI64 >> 32ll);
    res.y = (int)packedI64;
    return res;
}

Vec2i Vec2i::Add(Vec2i o) const
{
    return { x + o.x, y + o.y };
}

Vec2i Vec2i::AddValue(int add) const
{
    return { x + add, y + add };
}

Vec2i Vec2i::Subtract(Vec2i o) const
{
    return { x - o.x, y - o.y };
}

Vec2i Vec2i::SubtractValue(int sub) const
{
    return { x - sub, y - sub };
}

// Calculate distance between two vectors
float Vec2i::Distance(Vec2i o) const
{
    float result = zpl_sqrt((float)((x - o.x) * (x - o.x) + (y - o.y) * (y - o.y)));
    return result;
}

// Calculate square distance between two vectors
float Vec2i::SqrDistance(Vec2i o) const
{
    float result = (float)((x - o.x) * (x - o.x) + (y - o.y) * (y - o.y));
    return result;
}

int Vec2i::ManhattanDistance(Vec2i o) const
{
	int xLength = zpl_abs(x - o.x);
	int yLength = zpl_abs(y - o.y);
	return Max(xLength, yLength);
}

int Vec2i::ManhattanDistanceWithCosts(Vec2i o) const
{
	constexpr int MOVE_COST = 10;
	constexpr int DIAGONAL_COST = 14;
	int xLength = zpl_abs(x - o.x);
	int yLength = zpl_abs(y - o.y);
	int res = MOVE_COST * (xLength + yLength) + (DIAGONAL_COST - 2 * MOVE_COST) * (int)Min((float)xLength, (float)yLength);
	return res;
}

// Scale vector (multiply by value)
Vec2i Vec2i::Scale(int scale) const
{
    return { x * scale, y * scale };
}

// Multiply vector by vector
Vec2i Vec2i::Multiply(Vec2i o) const
{
    return { x * o.x, y * o.y };
}

// Negate vector
Vec2i Vec2i::Negate() const
{
    return { -x, -y };
}

Vec2i Vec2i::VecMin(Vec2i min) const
{
    Vec2i result;
    result.x = (x < min.x) ? min.x : x;
    result.y = (y < min.y) ? min.y : y;
    return result;
}

Vec2i Vec2i::VecMax(Vec2i max) const
{
    Vec2i result;
    result.x = (x > max.x) ? max.x : x;
    result.y = (y > max.y) ? max.y : y;
    return result;
}

// Divide vector by vector
Vec2i Vec2i::Divide(Vec2i o) const
{
    return { x / o.x, y / o.y };
}

_FORCE_INLINE_ bool operator==(Vec2i left, Vec2i right)
{
    return left.x == right.x && left.y == right.y;
}

_FORCE_INLINE_ bool operator!=(Vec2i left, Vec2i right)
{
    return left.x != right.x || left.y != right.y;
}

_FORCE_INLINE_ Vec2i operator+=(Vec2i left, Vec2i right)
{
    return left.Add(right);
}

_FORCE_INLINE_ Vec2i operator-=(Vec2i left, Vec2i right)
{
    return left.Subtract(right);
}

_FORCE_INLINE_ Vec2i operator*=(Vec2i left, Vec2i right)
{
    return left.Multiply(right);
}

_FORCE_INLINE_ Vec2i operator/=(Vec2i left, Vec2i right)
{
    return left.Divide(right);
}

_FORCE_INLINE_ Vec2i operator+(Vec2i left, Vec2i right)
{
    return left.Add(right);
}

_FORCE_INLINE_ Vec2i operator-(Vec2i left, Vec2i right)
{
    return left.Subtract(right);
}

_FORCE_INLINE_ Vec2i operator*(Vec2i left, Vec2i right)
{
    return left.Multiply(right);
}

_FORCE_INLINE_ Vec2i operator/(Vec2i left, Vec2i right)
{
    return left.Divide(right);
}
