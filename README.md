# open.mp Easing-Functions Component

An open.mp server component that adds textdraw easing and animation functionality.
It is essentially a component-based version of the original
[pawn-easing-functions](https://github.com/alexchwoj/pawn-easing-functions.git) by [alexchwoj](https://github.com/alexchwoj), with improvements and optimized performance for large-scale animations.

---

## Installation

1. Go to the **Releases** page and download the archive or binary matching your server platform (Windows `.dll`, Linux `.so`).
2. Extract it and place the component file into your open.mp server `components/` directory.
3. Copy `omp_easing.inc` into the include directory used by your Pawn compiler (e.g. `gamemodes/qawno/include/`).
4. Add

   ```pawn
   #include <omp_easing>
   ```

   to any script where you want to use easing native functions, then recompile your scripts.

---

## Functions

```pawn
// Utility
native Float:GetEasingValue(Float:t, easeType);
native Float:Lerp(Float:start, Float:end, Float:t);
native LerpColor(color1, color2, Float:t);

// Position
native PlayerText_MoveTo(playerid, PlayerText:textdraw, Float:x, Float:y, duration, easeType, bool:silent = false);
native PlayerText_MoveToX(playerid, PlayerText:textdraw, Float:x, duration, easeType, bool:silent = false);
native PlayerText_MoveToY(playerid, PlayerText:textdraw, Float:y, duration, easeType, bool:silent = false);

// Size
native PlayerText_MoveLetterSize(playerid, PlayerText:textdraw, Float:y, duration, easeType, bool:silent = false);
native PlayerText_MoveTextSize(playerid, PlayerText:textdraw, Float:x, duration, easeType, bool:silent = false);
native PlayerText_MoveSize(playerid, PlayerText:textdraw, Float:x, Float:y, duration, easeType, bool:silent = false);

// Color
native PlayerText_InterpolateColor(playerid, PlayerText:textdraw, color, duration, easeType, bool:silent = false);
native PlayerText_InterpolateBoxColor(playerid, PlayerText:textdraw, color, duration, easeType, bool:silent = false);
native PlayerText_InterpolateBGColor(playerid, PlayerText:textdraw, color, duration, easeType, bool:silent = false);

// General
native PlayerText_PlaceOnTop(playerid, PlayerText:textdraw);
native PlayerText_StopAnimation(animator_id);
native IsAnimationActive(animator_id);

// Debug and monitoring
native GetActiveAnimationsCount();
native GetAnimationStats(&totalCreated, &peakConcurrent, &totalCallbacks);
```

---

## Callback

```pawn
OnAnimatorFinish(playerid, animatorid, textdrawid, types);
```

---

## Easing Types

```
EASE_IN_SINE
EASE_OUT_SINE
EASE_IN_OUT_SINE
EASE_IN_QUAD
EASE_OUT_QUAD
EASE_IN_OUT_QUAD
EASE_IN_CUBIC
EASE_OUT_CUBIC
EASE_IN_OUT_CUBIC
EASE_IN_QUART
EASE_OUT_QUART
EASE_IN_OUT_QUART
EASE_IN_QUINT
EASE_OUT_QUINT
EASE_IN_OUT_QUINT
EASE_IN_EXPO
EASE_OUT_EXPO
EASE_IN_OUT_EXPO
EASE_IN_CIRC
EASE_OUT_CIRC
EASE_IN_OUT_CIRC
EASE_IN_BACK
EASE_OUT_BACK
EASE_IN_OUT_BACK
EASE_IN_ELASTIC
EASE_OUT_ELASTIC
EASE_IN_OUT_ELASTIC
EASE_IN_BOUNCE
EASE_OUT_BOUNCE
EASE_IN_OUT_BOUNCE
EASE_NONE
```

---

## Animation Enum
```
enum eAnimatorTypes 
{
	ANIMATOR_POSITION,
	ANIMATOR_LETTER_SIZE,
	ANIMATOR_TEXT_SIZE,
	ANIMATOR_FULL_SIZE,
	ANIMATOR_COLOR,
	ANIMATOR_BOX_COLOR,
	ANIMATOR_BACKGROUND_COLOR
}
```

You can preview each easing type visually here: [https://easings.net/](https://easings.net/)

---

## Credits
- [alexchwoj](https://github.com/alexchwoj) and all pawn-easing-functions contributors
- [AmyrAhmady](https://github.com/AmyrAhmady) for the open.mp component SDK
- [Fanorisky](https://github.com/Fanorisky) (me)
