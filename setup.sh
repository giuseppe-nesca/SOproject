#!/bin.bash

readonly tmpFile='./.tmp.conf'
readonly parametersFile='./parameters.conf'
readonly debugFile='./debug_flags.h'

debug[0]='DEBUG_RAND'
debug[1]='DEBUG_LRAND'
debug[2]='AUTOCLEAN_SEMS'
debug[3]='DEBUG_MARRIAGE'
debug[4]='PRINT_TIMER'
debug[5]='DEBUG_GESTORE'
debug[6]='PRINT_DEATH_INFOS'
debug[7]='DEBUG_KILLER'


readonly init_people='init_people ='
readonly genes='genes ='
readonly birth_death='birth_death ='
readonly sim_time='sim_time ='

function random {
    echo random values
    # set a max and min init_people value to preserve CPU and obtain a useful simulation at the same time
    readonly init_people_max=500
    readonly init_people_min=4
    init_people_value=$((($RANDOM % $init_people_max) + $init_people_min))
    echo $init_people $init_people_value >> $tmpFile

    genes_value=$(($RANDOM + 100))
    echo $genes $genes_value >> $tmpFile

    # set a max and min sim_time value to make everytime a useful and not annoying simulation
    readonly sim_time_max=60
    readonly sim_time_min=1
    sim_time_value=$((($RANDOM % $sim_time_max) + $sim_time_min))

    # set a max and min birth_death value to make everytime a useful simulation
    readonly birth_death_max=$sim_time_value
    readonly birth_death_min=1
    birth_death_value=$((($RANDOM % $birth_death_max) + $birth_death_min))
    echo $birth_death $birth_death_value >> $tmpFile
    echo $sim_time $sim_time_value >> $tmpFile

}

function isDigit {
    if [[ $1 =~ ^[0-9]+$ ]]; then
        # echo $init_people_value is digit
        return 0
    else
        echo $1 is not digit
        return 1
    fi
}

function manual {
    readonly message='Insert only digits without spaces or alphabetics'
    echo $message
    flag=true

    while $flag; do
        read -p "$init_people " init_people_value
        if isDigit $init_people_value; then
            break;
        fi
    done
    
    while $flag; do
        read -p "$genes " genes_value
        if isDigit $genes_value; then
            break;
        fi
    done
    
    while $flag; do
        read -p "$birth_death " birth_death_value
        if isDigit $birth_death_value; then
            break;
        fi
    done
    
    while $flag; do
        read -p "$sim_time " sim_time_value
        if isDigit $sim_time_value; then
            break;
        fi
    done

    echo $init_people $init_people_value >> $tmpFile
    echo $genes $genes_value >> $tmpFile
    echo $birth_death $birth_death_value >> $tmpFile
    echo $sim_time $sim_time_value >> $tmpFile
}

function select_debug {
    > $tmpFile
    echo '#ifndef SOPROJECTRC_DEBUG_FLAGS_H' >> $tmpFile
    echo '#define SOPROJECTRC_DEBUG_FLAGS_H' >> $tmpFile

    echo 'type y to set and n to ignore the following debug flags: (default: ignore)'
    for d in "${debug[@]}"; do
        read -p "$d [y,N] " yn
        case $yn in
        [Y,y]* ) echo "#define $d" >> $tmpFile;;
        *      ) echo "//#define $d" >> $tmpFile;;
        esac
    done
    echo '#endif //SOPROJECTRC_DEBUG_FLAGS_H' >> $tmpFile

    cat $tmpFile > $debugFile
    cat $debugFile; echo saved
}

function empty_debug {
    > $tmpFile
    echo '#ifndef SOPROJECTRC_DEBUG_FLAGS_H' >> $tmpFile
    echo '#define SOPROJECTRC_DEBUG_FLAGS_H' >> $tmpFile

    for d in "${debug[@]}"; do
        echo "//#define $d" >> $tmpFile
    done
    echo '#endif //SOPROJECTRC_DEBUG_FLAGS_H' >> $tmpFile

    cat $tmpFile > $debugFile
    cat $debugFile; echo saved
}

echo "Welcome on SOproject config generator"
> $tmpFile
flag=$true;
while $flag; do
    read -p "Do you like a random config [y/n]? (empty for current) " yn
    case $yn in
        [Yy]* ) random; break;;
        [Nn]* ) manual; break;;
        * ) cat $parametersFile > $tmpFile; break;;
    esac
done

echo; echo 'Result:'
cat $tmpFile
echo;
read -p "Confirm the config [Y/n]? " yn
case $yn in
    [Nn]* ) echo 'abort'; exit 0;;
    * ) echo confirmed;;
esac
cat $tmpFile > $parametersFile
echo "$parametersFile saved"

read -p "Do you want to edit special prints? [y,n] (leave empty for current options) " yn
case $yn in
    [Nn]* ) empty_debug;;
    [Yy]* ) echo 'select print'; select_debug;;
    *     ) echo current options;;
esac

(make)
if [ $? -ne 0 ]; then
    echo "make failed"
fi

exec ./gestore
# echo DONE