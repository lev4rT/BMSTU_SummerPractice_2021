# BMSTU_SummerPractice_2021
Летняя практика после 3 курса. Демон для UNIX-подобных систем. Опрашивает все доступные сетевые принтеры на наличие ошибок и низкого количества расходных материалов. Записывает информацию в `/var/log/syslog`.

## REQUIREMENTS
### smnp
`sudo apt-get install snmp`

## USAGE
###### BUILD
`sudo make`

###### RUN
`sudo make run`

###### VIEW LOGS
`make log`

###### STOP DAEMON
`sudo make kill`
