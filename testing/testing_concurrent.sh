#!/bin/bash

iterations=1000

cleanup() {
    echo "Cerrando conexiones..."
    for pid in "${pids[@]}"; do
        kill -TERM "$pid" 2>/dev/null
    done
    wait
    echo "Conexiones cerradas"
}

trap 'cleanup; exit 1' INT TERM EXIT

sleep_ms() {
    local duration=$(echo "$1 / 1000" | bc -l)
    sleep "$duration"
}

for ((i = 1; i <= iterations; i++)); do
    { printf "USER mdaneri\n"; sleep 10; printf "PASS pass123\n"; sleep 10; printf "LIST\n"; sleep 10;} | nc -C localhost 1110 >> test_concurrent_out.txt &
    pid=$!
    pids+=("$pid")
    printf "Connection $i created with pid: $pid\n"
    sleep_ms 5
done

# Wait for 20 seconds
sleep 20

# Clean up and close connections
cleanup

echo "All iterations completed."
