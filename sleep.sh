#! /bin/sh

time=0
for arg; do
	if ! echo $arg | grep -q '^[0-9][0-9]*[sSmMhH]\?$'; then
		echo "invalid argument: $arg" >&2
		exit 1
	fi

	set -- $(echo $arg | sed 's/^\([0-9][0-9]*\)\([sSmMhH]\?\)$/\1 \2/')
	num=${1:-0}
	unit=${2:-s}

	case $unit in
	[sS])
		time=$(($time + $num))
		;;
	[mM])
		time=$(($time + $num * 60))
		;;
	[hH])
		time=$(($time + $num * 3600))
		;;
	esac
done

sleep $time
