#! /bin/sh

time=0
for arg; do
	if ! echo $arg | grep -q '^[0-9][0-9]*[sSmMhH]\?$'; then
		echo "invalid argument: $arg" >&2
		exit 1
	fi
	unit=$(echo $arg | tr -d '[0-9]')
	test -z "$unit" && unit=s
	num=$(echo $arg | tr -cd '[0-9]')
	test -z "$num" && num=0

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
