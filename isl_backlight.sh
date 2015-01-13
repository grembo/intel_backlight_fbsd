#!/bin/sh

update_brightness()
(
	BRIGHTNESS=$(( `sysctl -n dev.isl.0.ir` / 10 ))
	if [ $BRIGHTNESS -lt 10 ]; then
		BRIGHTNESS=10  
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
				intel_backlight $LAST >/dev/null
				sleep 0.01
			done
		fi
	else
		DIFF=$(( $NEW - $LAST ))
		if [ $DIFF -gt 4 ]; then
			while [ $LAST -lt $NEW ]; do
				LAST=$(( LAST + 2 ))
				intel_backlight $LAST >/dev/null
				sleep 0.01
			done
		fi
	fi
done
