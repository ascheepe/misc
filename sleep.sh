#! /bin/sh

_is_valid_argument()
{
	if ! echo $1 | grep -q '^[0-9][0-9]*[sSmMhH]\?$'; then
		return 1
	fi

	return 0
}

_split_argument()
{
	echo $1 | sed 's/^\([0-9][0-9]*\)\([sSmMhH]\?\)$/\1 \2/'
}

sleep()
{
	_time=0
	for _arg; do
		if ! _is_valid_argument "$_arg"; then
			echo "invalid argument: $_arg" >&2
			return 1
		fi

		set -- $(_split_argument "$_arg")
		_num=${1:-0}
		_unit=${2:-s}

		case $_unit in
		[sS])
			_time=$(($_time + $_num))
			;;
		[mM])
			_time=$(($_time + $_num * 60))
			;;
		[hH])
			_time=$(($_time + $_num * 3600))
			;;
		esac
	done
	command sleep $_time
}
