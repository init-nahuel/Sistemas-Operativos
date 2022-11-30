cmd_/home/pss/Win/Modules2020-2/Hello/modules.order := {   echo /home/pss/Win/Modules2020-2/Hello/hello.ko; :; } | awk '!x[$$0]++' - > /home/pss/Win/Modules2020-2/Hello/modules.order
