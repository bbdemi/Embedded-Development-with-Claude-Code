# ISR Safety Review

Review all ISR-related code in this project for correctness and safety.

## Check for these issues

**Must-have**
- Every ISR function is marked `IRAM_ATTR` — crash on cache miss if missing
- Every variable shared between ISR and main loop is marked `volatile`
- No `digitalWrite` inside ISR — must use `gpio_set_level` from `driver/gpio.h`
- No `delay()` or `Serial.print()` inside ISR
- No dynamic memory allocation (malloc, new) inside ISR

**Timing**
- ISR body is short enough to complete before the next timer tick
- No floating-point math in ISR hot path
- No loops with unbounded length inside ISR

**Concurrency**
- Any multi-byte variable read in main loop that ISR writes should use `portENTER_CRITICAL`
- Flag variables set in ISR and read in loop should be `volatile bool`

## Output format

List each issue found as:
- File and line number
- What the problem is
- Exact fix (show the corrected line)

If no issues found, confirm it and say why the code is correct.
