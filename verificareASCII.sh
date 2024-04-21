source_dir="$1"  # directorul in care verificam fisierele
target_dir="$2"  # directorul pentru fisiere izolate

# functie in care verificam caracterele
contains_non_ascii() 
{
    if grep -qP '[^\x00-\x7F]' "$1"; then
        return 0  # am gasit caractere non-ASCII
    else
        return 1  # nu am gasit caractere non-ASCII
    fi
}

for file in "$source_dir"/*.txt; do
    if contains_non_ascii "$file"; then
        # mutam fisierele cu caractere non-ASCII
        mv "$file" "$target_dir"
        # schimbam drepturile fisierelor
        chmod 000 "$target_dir/$(basename "$file")"
    fi
done
