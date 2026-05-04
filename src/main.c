/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Stepper motor control for 28BYJ-48 via ULN2003 on pins D8-D11.
 *
 * 28BYJ-48 specs:
 *   Stride angle: 5.625°/64 (gear ratio 1:64)
 *   Steps per motor revolution: 64 (full-step)
 *   Steps per output shaft revolution: 64 * 64 = 4096 (full-step)
 *   With half-stepping (micro-step-res=2): 8192 half-steps per revolution
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/stepper.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pump, LOG_LEVEL_INF);

static const struct device *stepper = DEVICE_DT_GET(DT_ALIAS(stepper));

/* 28BYJ-48: 64 full steps * 64 gear ratio = 4096 full steps per output revolution.
 * With micro-step-res=2 (half-stepping): 8192 micro-steps per revolution. */
#define STEPS_PER_REV      4096
#define MICRO_STEP_RES     DT_PROP(DT_ALIAS(stepper), micro_step_res)
#define USTEPS_PER_REV     (STEPS_PER_REV * MICRO_STEP_RES)

/* Step interval in nanoseconds — controls motor speed.
 * 1200 µs per step ≈ ~1.6 RPM at the output shaft with half-stepping. */
#define STEP_INTERVAL_NS   1200000

static K_SEM_DEFINE(step_done, 0, 1);

static void stepper_cb(const struct device *dev, enum stepper_event event, void *user_data)
{
	if (event == STEPPER_EVENT_STEPS_COMPLETED) {
		k_sem_give(&step_done);
	}
}

int main(void)
{
	int ret;

	if (!device_is_ready(stepper)) {
		LOG_ERR("Stepper device not ready");
		return -ENODEV;
	}

	stepper_set_event_callback(stepper, stepper_cb, NULL);
	stepper_set_reference_position(stepper, 0);
	stepper_set_microstep_interval(stepper, STEP_INTERVAL_NS);

	stepper_enable(stepper);
	LOG_INF("Stepper enabled, %d micro-steps/rev", USTEPS_PER_REV);

	/* Rotate one full revolution forward, then one back, repeating. */
	while (1) {
		LOG_INF("Forward one revolution");
		ret = stepper_move_by(stepper, USTEPS_PER_REV);
		if (ret) {
			LOG_ERR("move_by failed: %d", ret);
			break;
		}
		k_sem_take(&step_done, K_FOREVER);

		LOG_INF("Reverse one revolution");
		ret = stepper_move_by(stepper, -USTEPS_PER_REV);
		if (ret) {
			LOG_ERR("move_by failed: %d", ret);
			break;
		}
		k_sem_take(&step_done, K_FOREVER);
	}

	stepper_disable(stepper);
	return 0;
}
