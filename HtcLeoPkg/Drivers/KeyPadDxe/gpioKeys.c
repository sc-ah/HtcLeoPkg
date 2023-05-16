/*
 * Copyright (c) 2012, Shantanu Gupta <shans95g@gmail.com>
 *
 * Based on google's gpio keypad driver, Adds support for
 * keys which are directly mapped to gpio pin.
 *
 * TODO: 
 *
 * BUGS:
 *		If board changes pdata->gpio_keys after init, strange stuff can happen
 *			This can be fixed by making a local copy of pdata, but no board
 *			would do that anyway or using const modifier.
 */

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>

#include <Protocol/BlockIo.h>
#include <Protocol/DevicePath.h>

#include <Library/keys.h>
#include <Library/gpio.h>
//#include <kernel/timer.h>
#include <Library/gpioKeys.h>

static int some_key_pressed;
static unsigned long gpio_keys_bitmap;

static struct timer gpio_keys_poll;
static struct gpio_keys_pdata *pdata;

static enum handler_return
gpio_keys_poll_fn(struct timer *timer, time_t now, void *arg)
{
	uint32_t i = 0;
	struct gpio_key *key = pdata->gpio_key;
	
	for(; ((i < pdata->nr_keys) && (key != NULL)); i++, key++) {
		int changed = 0;

		if (gpio_get(key->gpio_nr) ^ !!key->polarity) {
			changed = !bitmap_set(&gpio_keys_bitmap, i);
		} else {
			changed = bitmap_clear(&gpio_keys_bitmap, i);
		}

		if (changed) {
			int state = bitmap_test(&gpio_keys_bitmap, i);

			keys_post_event(key->key_code, state);
			if (key->notify_fn) {
				key->notify_fn(key->key_code, state);
			}
			if(state) {
				some_key_pressed++;
			} else if(some_key_pressed) {
				some_key_pressed--;
			}
		}
	}

	if (some_key_pressed) {
	//	timer_set_oneshot(timer, pdata->debounce_time, gpio_keys_poll_fn, NULL);
	} else {
	//	timer_set_oneshot(timer, pdata->poll_time, gpio_keys_poll_fn, NULL);		
	}
	
	return 1;
}

void gpio_keys_init(struct gpio_keys_pdata *kpdata)
{
	if(kpdata == NULL) {
		printf("FAILED: platform data is NULL!\n");
		return;
	} else if (pdata != NULL) {
		printf("FAILED: platform data is set already!\n");
		return;
	} else if (!kpdata->nr_keys || (!(kpdata->nr_keys < BITMAP_BITS_PER_WORD)) ) {
		printf("WARN: %d keys not supported, First 32 keys will be served.\n", (unsigned)kpdata->nr_keys);
		kpdata->nr_keys = 32;
	}

	pdata = kpdata;
	
	some_key_pressed = 0;
	memset(&gpio_keys_bitmap, 0, sizeof(unsigned long));

//	timer_initialize(&gpio_keys_poll);
//	timer_set_oneshot(&gpio_keys_poll, 0, gpio_keys_poll_fn, NULL);
}