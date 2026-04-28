#ifndef PLATFORM_H
#define PLATFORM_H

#include "hal/hal.h"

/* Pin identifiers for this platform.
 * The numeric values must match the switch cases in hal_impl.c. */
#define PIN_BUTTON ((hal_pin_t)0) /* B1 — PC13, active-LOW */
#define PIN_LED    ((hal_pin_t)1) /* LD2 — PA5 */

#endif /* PLATFORM_H */
