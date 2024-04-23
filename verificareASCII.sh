source_file="$1"  # fisier care trebuie verificat
target_dir="$2"  # director pentru fisiere izolate

# verificam daca fisierul contine caractere non-ASCII sau cuvinte specifice
contains_non_ascii_or_words() {
    if grep -qP '[^\x00-\x7F]' "$1" || grep -qE 'corrupted|malicious|malware|dangerous|risk|attack' "$1"; then
        #obtinem numarul de linii, cuvinte si caractere
        lines=$(wc -l < "$1")
        words=$(wc -w < "$1")
        characters=$(wc -m < "$1")
        # mutam fisierul in target_dir
        mv "$1" "$target_dir"
        chmod 000 "$target_dir/$(basename "$1")"
        # afisam numarul de linii, cuvinte si caractere
        echo "$(basename "$1"): $lines lines; $words words; $characters characters has been isolated"
    fi
}

#chmod 777 "$source_file"
#contains_non_ascii_or_words "$source_file"

if [ "$(stat -c "%a" "$source_file")" -eq "000" ]; then
    chmod 777 "$source_file"
    # Call a function to check for non-ASCII characters or specific words
    contains_non_ascii_or_words "$source_file"
fi