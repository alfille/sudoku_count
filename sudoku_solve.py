import sys
import tkinter as tk
import tkinter.font as tkfont
import argparse
import ctypes
import platform

class Sudoku(tk.Frame):
	SUBSIZE = 3
	color=["dark gray","white"]
	
	def __init__(self, master=None, subsize=3):
		super().__init__(master)
		self.SUBSIZE = subsize
		self.SIZE = self.SUBSIZE * self.SUBSIZE
		self.TOTALSIZE = self.SIZE * self.SIZE
		self.master = master
		self.font = tkfont.Font(weight="bold",size=14)
		#self.pack()
		self.create_widgets()
		self.create_menu()
		self.win.update()
		self.set_win_sizes()

	def set_win_sizes( self ):
		self.tilex=self.but[0][0].winfo_width()
		self.tiley=self.but[0][0].winfo_height()
		self.popx = self.tilex*self.SUBSIZE
		self.popy = self.tiley*self.SUBSIZE
		self.winx = self.win.winfo_screenwidth()
		self.winy = self.win.winfo_screenheight()
		
	
	def set_square(self,i,j,n):
		self.but[i][j].configure(text=n)
		self.sq_popup_done( i , j )
		
	def sq_popup_done( self, i, j ):
		self.pop.destroy()
		self.but[i][j].configure(relief="raised")
			
	
	def sq_popup( self,i,j):
		self.but[i][j].configure(relief="sunken")
		self.pop=tk.Toplevel()
		
		# figure location
		self.win.update()
		x = self.but[i][j].winfo_rootx()+self.tilex
		y = self.but[i][j].winfo_rooty()
		if x+self.popx > self.winx:
			x -= self.tilex+self.popx
		if x<0:
			x=0
		if y+self.popy > self.winy:
			y = self.winy - self.popy
		self.pop.geometry('+%d+%d' % (x,y) )
		
		for si in range(self.SUBSIZE):
			for sj in range(self.SUBSIZE):
				n = si*self.SUBSIZE+sj+1				
				tk.Button(self.pop,text=str(n),borderwidth=4,height=2,width=3,font=self.font,command=lambda i=i,j=j,n=str(n): self.set_square(i,j,n)).grid(row=si,column=sj)
		tk.Button(self.pop,text="Clear",borderwidth=4,height=2,font=self.font,command=lambda i=i,j=j,n=" ": self.set_square(i,j,n)).grid(columnspan=self.SUBSIZE,sticky="EW")
		tk.Button(self.pop,text="Back",borderwidth=4,height=2,font=self.font,command=lambda i=i,j=j: self.sq_popup_done(i,j)).grid(columnspan=self.SUBSIZE,sticky="EW")
		self.pop.grab_set()

	def clear(self):
		for i in range(self.SIZE):
			for j in range(self.SIZE):
				self.but[i][j].configure(text=" ")
				
	def solve(self):
		arr = (ctypes.c_int * self.TOTALSIZE)(-1)
		k = 0
		for i in range(self.SIZE):
			for j in range(self.SIZE):
				arr[k] = -1 # default blank
				t = self.but[i][j].cget('text')
				if t != " ":
					arr[k] = int(t)-1 # 0-based values for squares
					#print(i,j,k,arr[k])
				#print("{} -> {}".format(k,arr[k]))
				k += 1
		solve_lib.Solve(arr)
		k = 0
		for i in range(self.SIZE):
			for j in range(self.SIZE):
				if arr[k] >= 0 :
					self.but[i][j].configure(text=str(arr[k]+1)) # 1-based text values
				else:
					self.but[i][j].configure(text=" ")
				k += 1
	
	def Quit(self):
		self.master.destroy()
	
	def create_widgets(self):
		self.win = tk.Frame(self.master,borderwidth=2,relief="flat",background="white")
		self.win.pack(side="top")
		self.buttons=tk.Frame(self.master,borderwidth=2,relief="flat",background="white")
		tk.Button(self.buttons,text="Solve",command=self.solve).pack(side="left")
		tk.Button(self.buttons,text="Clear",command=self.clear).pack(side="left")
		tk.Button(self.buttons,text="Exit",command=self.Quit).pack(side="left")
		self.buttons.pack(side="bottom")
	
		self.but = [[0 for i in range(self.SIZE)] for j in range(self.SIZE)]
		for si in range(self.SUBSIZE):
			for sj in range(self.SUBSIZE):
				f = tk.Frame(self.win,background=self.color[(si+sj)%2],borderwidth=2,relief="flat")
				f.grid(row=si,column=sj)
				for ssi in range(self.SUBSIZE):
					for ssj in range(self.SUBSIZE):
						i = si*self.SUBSIZE+ssi
						j = sj*self.SUBSIZE+ssj
						self.but[i][j] = tk.Button(f,text=str(1+(i+j)%self.SIZE),borderwidth=4,height=2,width=3,font=self.font,command=lambda i=i,j=j: self.sq_popup(i,j))
						self.but[i][j].grid(row=ssi, column=ssj)
		#self.win = tk.Button(self)
		#self.win["text"] = "Hello World\n(click me)"
		#self.win["command"] = self.say_hi
		#self.win.pack(side="top")

		#self.quit = tk.Button(self, text="QUIT", fg="red",
		#					  command=self.master.destroy)
		#self.quit.pack(side="bottom")
			
	def about(self):
		print("Sudoku Solve by Paul Alfille 2020")
	
	def create_menu(self):
		self.menu = tk.Menu(self.master,tearoff=0)
		self.helpmenu = tk.Menu(self.menu,tearoff=0)
		self.menu.add_cascade(label="Help",menu=self.helpmenu)
		self.helpmenu.add_command(label="About",command=self.about)
		self.master.config(menu=self.menu)

def main(args):

	# Shared C library
	lib_base = "./" #location
	lib_base += "sudoku_lib" # base name
	
	lib_base += str(Sudoku.SUBSIZE*Sudoku.SUBSIZE)	

	# get the right filename
	if platform.uname()[0] == "Windows":
		lib_base += ".dll" 
	if platform.uname()[0] == "Linux":
		lib_base += ".so" 
	else:
		lib_base += ".dylib" 

	# load library
	global solve_lib
	solve_lib = ctypes.cdll.LoadLibrary(lib_base)
	
	 
	Sudoku(master=tk.Tk()).mainloop()

if __name__ == "__main__":
	# execute only if run as a script
	sys.exit(main(sys.argv))
