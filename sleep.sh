#! /bin/sh

sleep()
{
	local value unit
	local time=0 arg

	for arg; do
		if ! echo $arg | grep -q '^[0-9][0-9]*[sSmMhH]\?$'; then
			echo "sleep: invalid time interval '$arg'" >&2
			return 1
		fi

		set -- $(echo $arg | \
		    sed 's/^\([0-9][0-9]*\)\([sSmMhH]\?\)$/\1 \2/')

		value=$1
		unit=${2:-s}

		case $unit in
		[sS])
			time=$(($time + $value))
			;;
		[mM])
			time=$(($time + $value * 60))
			;;
		[hH])
			time=$(($time + $value * 3600))
			;;
		esac
	done

	command sleep $time
}
