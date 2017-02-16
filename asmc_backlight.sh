#!/bin/sh

CONTROL_MAX=100
SENSOR_MAX=255
ADJUSTMENT=30
DEBUG=0

update_brightness()
(
	BRIGHTNESS_RIGHT=$(( `sysctl -n dev.asmc.0.light.right` ))
	BRIGHTNESS_LEFT=$(( `sysctl -n dev.asmc.0.light.left` ))
	BRIGHTNESS=0;
	if [ $BRIGHTNESS_LEFT -gt 0 ] & [ $BRIGHTNESS_RIGHT -gt 0 ]; then
		BRIGHTNESS=$((($BRIGHTNESS_LEFT + $BRIGHTNESS_RIGHT) / 2))
	elif [ $BRIGHTNESS_LEFT -gt 0 ]; then
		BRIGHTNESS=$BRIGHTNESS_LEFT
	elif [ $BRIGHTNESS_RIGHT -gt 0 ]; then
		BRIGHTNESS=$BRIGHTNESS_RIGHT
	fi

	# Sensor value 0-255
	# Convert to control value 0-100

	BRIGHTNESS=$((($BRIGHTNESS * $CONTROL_MAX) / $SENSOR_MAX + $ADJUSTMENT))


	if [ $BRIGHTNESS -lt 15 ]; then
		BRIGHTNESS=15
	fi
	if [ $BRIGHTNESS -gt 100 ]; then
		BRIGHTNESS=100
	fi
	return $BRIGHTNESS
)

update_brightness
LAST=$?
intel_backlight $LAST >/dev/null

while sleep 1; do
	update_brightness
	NEW=$?
	if [ $LAST -gt $NEW ]; then
		DIFF=$(( $LAST - $NEW ))
		if [ $DIFF -gt 4 ]; then
			while [ $LAST -gt $NEW ]; do
				LAST=$(( LAST - 2 ))
				if [ $DEBUG -eq 1 ]; then
					echo Setting value $LAST \(1\)
				fi
				intel_backlight $LAST >/dev/null
				sleep 0.01
			done
		fi
	else
		DIFF=$(( $NEW - $LAST ))
		if [ $DIFF -gt 4 ]; then
			while [ $LAST -lt $NEW ]; do
				LAST=$(( LAST + 2 ))
				if [ $DEBUG -eq 1 ]; then
					echo Setting value $LAST \(2\)
				fi
				intel_backlight $LAST >/dev/null
				sleep 0.01
			done
		fi
	fi
done
