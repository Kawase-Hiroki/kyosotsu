set terminal pngcairo
set output "image/out_male.png"
set datafile separator ","
set xlabel "Disclosure"
set ylabel "Matches"
plot "male_data.csv" using 2:6 with points title "Male"
system("echo plot finished")
system("ls -lh image/out_male.png")