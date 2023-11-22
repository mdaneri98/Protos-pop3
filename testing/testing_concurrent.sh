#!/bin/bash

# Specify the number of iterations
iterations=1000

# Array to store background job PIDs
pids=()

# Function to clean up and close connections
cleanup() {
    echo "Closing connections..."
    for pid in "${pids[@]}"; do
        kill -TERM "$pid" 2>/dev/null
    done
    wait
    echo "Connections closed."
}

sleep_ms() {
    local duration=$(echo "$1 / 1000" | bc -l)
    sleep "$duration"
}

# Trap signals to ensure cleanup on exit
trap 'cleanup; exit 1' INT TERM EXIT

# Loop to run the script
for ((i = 1; i <= iterations; i++)); do
    # Run the script and store the PID
    (printf "USER mdaneri\nPASS pass123\nLIST\n" | ncat -C localhost 1110) >> test_concurrent_out.txt &
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
