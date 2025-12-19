#!/bin/bash

echo "Тестирование параллельного определителя"
echo "========================================"

gcc -O2 -pthread -o determinant_calculator determinant_calculator.c -lm

SIZE=7

echo "Матрица: ${SIZE}×${SIZE}"
echo ""

echo "Потоки  | Время (с)    | Ускорение | Эффективность"
echo "--------|--------------|-----------|--------------"

BASE_TIME=0
for t in 1 2 3 4 6 8
do
    RESULT=$(./determinant_calculator $t $SIZE 2>/dev/null)
    
    TIME_STR=$(echo "$RESULT" | grep "Время работы:")
    TIME=$(echo "$TIME_STR" | awk '{print $3}')
    
    if [ -z "$TIME" ]; then
        TIME_STR=$(echo "$RESULT" | grep "Time:")
        TIME=$(echo "$TIME_STR" | awk '{print $2}')
    fi
    
    if [ -z "$TIME" ]; then
        echo "Ошибка: не удалось извлечь время для $t потоков"
        echo "Вывод программы был:"
        echo "$RESULT"
        continue
    fi
    
    TIME=${TIME/,/.}

    if [ $t -eq 1 ]; then
        BASE_TIME=$TIME
        SPEEDUP=1.00
        EFF=1.000
        printf "   %2d   |  %-10s  |  %.2fx    |  %.3f\n" $t $TIME $SPEEDUP $EFF
    else
        SPEEDUP=$(awk -v base="$BASE_TIME" -v time="$TIME" 'BEGIN {printf "%.2f", base/time}')
        EFF=$(awk -v speedup="$SPEEDUP" -v t="$t" 'BEGIN {printf "%.3f", speedup/t}')
        printf "   %2d   |  %-10s  |  %.2fx    |  %.3f\n" $t $TIME $SPEEDUP $EFF
    fi
done
