#pragma once

#include <cstddef>
#include <deque>
#include <set>

#include <IO/WriteBuffer.h>
#include <IO/ReadBuffer.h>

namespace DB
{


/** A pair of marks that defines the range of rows in a part. Specifically,
 * the range has the form [begin * index_granularity, end * index_granularity).
 */
struct MarkRange
{
    size_t begin;
    size_t end;

    MarkRange() = default;
    MarkRange(const size_t begin_, const size_t end_) : begin{begin_}, end{end_} {}

    size_t getNumberOfMarks() const;

    bool operator==(const MarkRange & rhs) const;
    bool operator<(const MarkRange & rhs) const;
};

struct MarkRanges : public std::deque<MarkRange>
{
    using std::deque<MarkRange>::deque;

    size_t getNumberOfMarks() const;

    void serialize(WriteBuffer & out) const;
    void describe(WriteBuffer & out) const;
    void deserialize(ReadBuffer & in);
};

/** Get max range.end from ranges.
 */
size_t getLastMark(const MarkRanges & ranges);

std::string toString(const MarkRanges & ranges);

}
