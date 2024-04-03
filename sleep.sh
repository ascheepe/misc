#! /bin/sh

_is_valid_argument()
{
	if echo $1 | grep -q '^[0-9][0-9]*[sSmMhH]\?$'; then
		return 0
	fi

	return 1
}

_arg_to_time()
{
	set -- $(echo $1 | sed 's/^\([0-9][0-9]*\)\([sSmMhH]\?\)$/\1 \2/')
	_num=$1
	_unit=${2:-s}

	case $_unit in
	[sS])
		echo $_num
		;;
	[mM])
		echo $(($_num * 60))
		;;
	[hH])
		echo $(($_num * 3600))
		;;
	esac
}

sleep()
{
	_time=0
	for _arg; do
		if ! _is_valid_argument "$_arg"; then
			echo "sleep: invalid time interval '$_arg'" >&2
			return 1
		fi

		_time=$(($_time + $(_arg_to_time $_arg)))

	done
	command sleep $_time
}
