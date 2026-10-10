#include "FillGenerator.h"

namespace bm
{

const char* fillStyleName (FillStyle style) noexcept
{
    switch (style)
    {
        case FillStyle::SnareRoll:   return "Snare roll";
        case FillStyle::TomCascade:  return "Tom cascade";
        case FillStyle::SnareToToms: return "Snare into toms";
        case FillStyle::Unison:      return "Kick + snare stabs";
        case FillStyle::NumStyles:   break;
    }
    return "?";
}

namespace
{
    // Velocity rising from 'from' to 'to' across n steps.
    int ramp (int step, int n, int from, int to)
    {
        return n <= 1 ? to : from + (to - from) * step / (n - 1);
    }

    bool isOnEighth (const Layout& l, int step)
    {
        const double pos = step * l.stepLength();
        return std::abs (pos * 2.0 - std::round (pos * 2.0)) < 1.0e-6;
    }

    // Spread the steps [from, to) over the drums, high to low, with slightly uneven groups.
    void cascade (Pattern& p, const std::vector<Lane>& drums, int from, int to, juce::Random& rng)
    {
        const int n = to - from;
        if (n <= 0 || drums.empty())
            return;

        std::vector<int> sizes (drums.size(), n / (int) drums.size());
        for (int i = 0; i < n % (int) drums.size(); ++i)
            ++sizes[(size_t) rng.nextInt ((int) drums.size())];
        if (drums.size() > 1 && rng.nextBool())   // borrow a step from one drum for another
        {
            const auto a = (size_t) rng.nextInt ((int) drums.size()), b = (size_t) rng.nextInt ((int) drums.size());
            if (sizes[a] > 1) { --sizes[a]; ++sizes[b]; }
        }
        for (size_t d = 0; sizes.back() == 0 && d + 1 < sizes.size(); ++d)   // always end on the lowest drum
            if (sizes[d] > 0) { --sizes[d]; ++sizes.back(); }

        int step = from;
        for (size_t d = 0; d < drums.size(); ++d)
            for (int i = 0; i < sizes[d]; ++i, ++step)
                p.set (drums[d], step, ramp (step, to, 92, 118));
        p.set (drums.back(), to - 1, 124);
    }

    std::vector<Lane> pickToms (juce::Random& rng)
    {
        std::vector<Lane> toms { Tom1, Tom2, Tom3, Floor };
        if (rng.nextInt (3) == 0)
            toms.erase (toms.begin() + 1 + rng.nextInt (2));   // sometimes skip a rack tom
        return toms;
    }
}

void generateFill (Pattern& p, FillStyle style, juce::Random& rng)
{
    const auto l = p.layout();
    const int n = l.numSteps();
    p.clear();

    switch (style)
    {
        case FillStyle::SnareRoll:
            for (int k = 0; k < n; ++k)
                p.set (Snare, k, ramp (k, n, 55 + rng.nextInt (20), 124));
            for (int k = 0; k < n; ++k)
                if (isOnEighth (l, k) && rng.nextInt (4) != 0)
                    p.set (Kick, k, 100);
            break;

        case FillStyle::TomCascade:
        {
            auto drums = pickToms (rng);
            if (rng.nextBool())
                drums.insert (drums.begin(), Snare);
            cascade (p, drums, 0, n, rng);
            for (int k = 0; k < n; k += 2)
                p.set (Kick, k, 100);
            break;
        }

        case FillStyle::SnareToToms:
        {
            const int split = juce::jlimit (1, juce::jmax (1, n - 1), n / 2 + (rng.nextBool() ? 0 : (n >= 8 ? -1 : 0)));
            for (int k = 0; k < split; ++k)
                p.set (Snare, k, ramp (k, split, 80, 110));
            cascade (p, pickToms (rng), split, n, rng);
            for (int k = 0; k < n; ++k)
                if (isOnEighth (l, k))
                    p.set (Kick, k, 100);
            break;
        }

        case FillStyle::Unison:
        {
            static const char* const shapes[] = { "x.x.", "xx.x", "x.xx", "xxx." };
            const juce::String shape (shapes[rng.nextInt ((int) std::size (shapes))]);
            for (int k = 0; k < n; ++k)
                if (shape[k % 4] == 'x' || k == n - 1)
                {
                    const int v = k == n - 1 ? 124 : 105;
                    p.set (Snare, k, v);
                    p.set (Kick, k, v);
                    if (k == 0 || k == n - 1 || rng.nextInt (3) == 0)
                        p.set (Crash, k, v);
                }
            break;
        }

        case FillStyle::NumStyles:
            break;
    }
}

} // namespace bm
