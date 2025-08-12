#!/bin/bash

# AUTHOR: AARAV GIRI

# Path to the input file containing syscall data
INPUT_FILE="inp_list.txt"
# Path to the template source file
TEMPLATE_FILE="syscall_trap_v2.c"
# Path to the Makefile
MAKEFILE="Makefile"

# Initialize Makefile dependencies string
MAKEFILE_DEPENDENCIES=""
# Line number counter
N=0

# Read the input file line by line
while IFS= read -r line; do
    # Trim leading and trailing whitespaces from the line
    line=$(echo "$line" | xargs)

    # Skip empty lines or lines with only spaces
    if [[ -z "$line" ]]; then
        continue
    fi

    # Split the line into syscall_name and namespace_list
    IFS=',' read -r syscall_name namespace_list <<< "$line"

    # Skip lines with no syscall_name or namespace_list
    if [[ -z "$syscall_name" || -z "$namespace_list" ]]; then
        continue
    fi

    # Increment the line counter
    ((N++))

    # Split namespaces into an array
    IFS=',' read -r -a namespaces <<< "$namespace_list"

    # Calculate the number of namespaces (PID_QSIZE)
    pid_qsize=${#namespaces[@]}

    # Construct the TARGET_PID_NAMESPACE array string
    target_pid_namespace="int TARGET_PID_NAMESPACE[] = {"
    for namespace in "${namespaces[@]}"; do
        target_pid_namespace+="$namespace, "
    done
    # Remove trailing comma and space
    target_pid_namespace=${target_pid_namespace%, }

    target_pid_namespace+="};"

    # Create a new file name based on syscall name
    output_file="${syscall_name}_trap_v2.c"
    object_file="${syscall_name}_trap_v2.o"

    # Check if the file already exists
    if [ -f "$output_file" ]; then
        echo "File $output_file already exists. Removing it."
        rm -f "$output_file" # Remove the old source file
    fi

    # Check if the corresponding object file already exists
    if [ -f "$object_file" ]; then
        echo "Object file $object_file already exists. Removing it."
        rm -f "$object_file" # Remove the old object file
    fi

    # Copy the template file content to the new file
    cp "$TEMPLATE_FILE" "$output_file"

    # Replace the #define PID_QSIZE and int TARGET_PID_NAMESPACE in the new file
    sed -i "s/#define PID_QSIZE .*/#define PID_QSIZE $pid_qsize/" "$output_file"
    sed -i "s|int TARGET_PID_NAMESPACE.*|$target_pid_namespace|" "$output_file"

    # Replace every occurrence of 'SYSCALL' with the syscall_name,
    # excluding lines where 'SYSCALL' is after a '//'
    awk -v syscall_name="$syscall_name" '
    {
        if ($0 ~ "//") {
            split($0, parts, "//");
            gsub("SYSCALL", syscall_name, parts[1]);
            $0 = parts[1] "//" parts[2];
        } else {
            gsub("SYSCALL", syscall_name);
        }
        print;
    }' "$output_file" > temp_file && mv temp_file "$output_file"

    echo "Generated $output_file"

    # Add the object file to the MAKEFILE_DEPENDENCIES string
    MAKEFILE_DEPENDENCIES+="${syscall_name}_trap_v2.o "
done < "$INPUT_FILE"

# After processing, check if N is still 0 (i.e., no lines were processed)
if [[ $N -eq 0 ]]; then
    # If inp_list.txt is empty or contains only invalid lines, overwrite the obj-m line with only $(MODULE_NAME).o
    echo "inp_list.txt is empty or has no valid lines. Keeping only $(MODULE_NAME).o in Makefile."
    sed -i "/^obj-m :=/ s/.*/obj-m := $(MODULE_NAME).o/" "$MAKEFILE"
else
    # If inp_list.txt is not empty, overwrite the obj-m line with dependencies
    echo "inp_list.txt is not empty. Overwriting obj-m line in Makefile with dependencies..."
    MAKEFILE_DEPENDENCIES=$(echo "$MAKEFILE_DEPENDENCIES" | sed 's/ *$//')  # Remove any trailing space
    sed -i "/^obj-m :=/ s/.*/obj-m := $MAKEFILE_DEPENDENCIES/" "$MAKEFILE"
    echo "Makefile updated with dependencies: $MAKEFILE_DEPENDENCIES"
fi

echo "Files written. Makefile updated."

# Prompt to compile files
echo "Would you like to compile the files? (enter 'y' if yes):"
read -r compile_response
if [[ "$compile_response" != "y" ]]; then
    echo "Exiting."
    exit 0
else
    # Compile using make
    if make; then
        echo "Compilation occurred successfully."
    else
        echo "Found errors on compilation, exiting."
        exit 1
    fi
fi

# Prompt to insert modules
echo "Would you like to insert the modules now? (enter 'y' if yes):"
read -r insert_response
if [[ "$insert_response" != "y" ]]; then
    echo "Exiting."
    exit 0
else
    # Insert each module
    while IFS= read -r line; do
        syscall_name=$(echo "$line" | cut -d',' -f1 | xargs)
        if [[ -n "$syscall_name" ]]; then
            module="${syscall_name}_trap_v2.ko"
            echo "Inserting module $module..."
            if sudo insmod "$module"; then
                echo "Successfully inserted module $module"
            else
                echo "Inserting $module was unsuccessful"
            fi
        fi
    done < "$INPUT_FILE"
    echo "Successfully processed all the modules."
fi