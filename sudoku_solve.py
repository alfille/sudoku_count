import sys
import tkinter as tk
import tkinter.font as tkfont
import argparse
import ctypes
import platform
import signal

def signal_handler(signal, frame):
    print("\nForced end\n")
    sys.exit(0)

class Persist(tk.Frame):
	SUBSIZE = 3
	X = False
	Window = False


class Sudoku(tk.Frame):
	#color=["dark gray","white"]
	color=["dark blue","yellow"]
	
	def __init__(self, master=None):
		super().__init__(master)
		self.SIZE = Persist.SUBSIZE * Persist.SUBSIZE
		self.TOTALSIZE = self.SIZE * self.SIZE
		self.master = master
		self.font = tkfont.Font(weight="bold",size=14)
		#self.pack()

		self.X = tk.BooleanVar()
		self.X.set(Persist.X)
		self.Window = tk.BooleanVar()
		self.Window.set(Persist.Window)

		self.create_widgets()
		self.create_menu()
		self.win.update()
		self.set_win_sizes()

	def set_win_sizes( self ):
		# needed for popup
		self.tilex=self.but[0][0].winfo_width()
		self.tiley=self.but[0][0].winfo_height()
		self.popx = self.tilex*Persist.SUBSIZE
		self.popy = self.tiley*Persist.SUBSIZE
		self.winx = self.win.winfo_screenwidth()
		self.winy = self.win.winfo_screenheight()
		
	
	def set_square(self,i,j,n):
		self.but[i][j].configure(text=n)
		self.sq_popup_done( i , j )
		
	def sq_popup_done( self, i, j ):
		self.pop.destroy()
		self.but[i][j].configure(relief="raised")
		self.status.configure(text="Edit mode")
			
	
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
		
		for si in range(Persist.SUBSIZE):
			for sj in range(Persist.SUBSIZE):
				n = si*Persist.SUBSIZE+sj+1				
				tk.Button(self.pop,text=str(n),borderwidth=4,height=2,width=3,font=self.font,command=lambda i=i,j=j,n=str(n): self.set_square(i,j,n)).grid(row=si,column=sj)
		tk.Button(self.pop,text="Clear",borderwidth=4,height=2,font=self.font,command=lambda i=i,j=j,n=" ": self.set_square(i,j,n)).grid(columnspan=Persist.SUBSIZE,sticky="EW")
		tk.Button(self.pop,text="Back",borderwidth=4,height=2,font=self.font,command=lambda i=i,j=j: self.sq_popup_done(i,j)).grid(columnspan=Persist.SUBSIZE,sticky="EW")
		self.pop.grab_set()

	def clear(self):
		self.status.configure(text="Clearing...")
		for i in range(self.SIZE):
			for j in range(self.SIZE):
				self.but[i][j].configure(text=" ")
		self.status.configure(text="Cleared")
				
	def solve(self):
		self.status.configure(text="Solving...")
		self.master.update()
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
		if Persist.X:
			x=1
		else:
			x=0
		if Persist.Window:
			w=1
		else:
			w=0
		if solve_lib.Solve(x,w,arr)==1:
			self.status.configure(text="Successfully solved")
		else:
			self.status.configure(text="Not solvable")
		k = 0
		for i in range(self.SIZE):
			for j in range(self.SIZE):
				if arr[k] >= 0 :
					self.but[i][j].configure(text=str(arr[k]+1)) # 1-based text values
				else:
					self.but[i][j].configure(text=" ")
				k += 1
	
	def Quit(self):
		sys.exit()
		self.master.quit()
	
	def create_widgets(self):
		self.win = tk.Frame(self.master,borderwidth=2,relief="flat",background="white")
		self.win.pack(side="top")
		self.buttons=tk.Frame(self.master,borderwidth=2,relief="flat",background="white")
		if Persist.SUBSIZE > 2:
			tk.Label(self.buttons,text=" {0}x{0} ".format(self.SIZE),relief="sunken",anchor="c",font=self.font).pack(side="left",fill=tk.Y)
		tk.Button(self.buttons,text="Solve",command=self.solve,font=self.font).pack(side="left")
		tk.Button(self.buttons,text="Clear",command=self.clear,font=self.font).pack(side="left")
		tk.Button(self.buttons,text="Exit",command=self.Quit,font=self.font).pack(side="left")
		self.status = tk.Label(self.buttons,text="Edit mode",relief="sunken",anchor="e")
		self.status.pack(side="left",fill="both",expand=1)
		self.buttons.pack(side="bottom",fill=tk.X)
	
		self.but = [[0 for i in range(self.SIZE)] for j in range(self.SIZE)]
		for si in range(Persist.SUBSIZE):
			for sj in range(Persist.SUBSIZE):
				f = tk.Frame(self.win,background=self.color[(si+sj)%2],borderwidth=3,relief="flat")
				f.grid(row=si,column=sj)
				for ssi in range(Persist.SUBSIZE):
					for ssj in range(Persist.SUBSIZE):
						i = si*Persist.SUBSIZE+ssi
						j = sj*Persist.SUBSIZE+ssj
						self.but[i][j] = tk.Button(f,text=str(1+(i+j)%self.SIZE),borderwidth=4,height=2,width=3,font=self.font,command=lambda i=i,j=j: self.sq_popup(i,j))
						self.but[i][j].grid(row=ssi, column=ssj)
						if Persist.X and ((i==j) or (i == self.SIZE-j-1)):
							self.but[i][j].configure(background="light yellow")
						if Persist.Window:
							if (i % (Persist.SUBSIZE+1) > 0) and (j % (Persist.SUBSIZE+1) > 0):
								self.but[i][j].configure(background="aquamarine")
								if Persist.X and ((i==j) or (i == self.SIZE-j-1)):
									self.but[i][j].configure(background="pale green")
								
			
	def about(self):
		print("Sudoku Solve by Paul Alfille 2020")
		
	def Size(self) :
		if ( self.ss_choose != Persist.SUBSIZE ):
			Persist.SUBSIZE = self.ss_choose.get()
			self.master.destroy()
	
	def Option(self):
		if Persist.X != self.X.get():
			Persist.X = self.X.get()
			self.master.destroy()
		if Persist.Window != self.Window.get():
			Persist.Window = self.Window.get()
			self.master.destroy()


	def create_menu(self):
		self.menu = tk.Menu(self.master,tearoff=0)

		self.filemenu = tk.Menu(self.menu,tearoff=0)
		self.menu.add_cascade(label="File",menu=self.filemenu,font=self.font)
		self.filemenu.add_command(label="Solve",command=self.solve,font=self.font)
		self.filemenu.add_command(label="Clear",command=self.clear,font=self.font)
		self.filemenu.add_command(label="Exit",command=self.Quit,font=self.font)

		self.sizemenu = tk.Menu(self.menu,tearoff=0)
		self.menu.add_cascade(label="Size",menu=self.sizemenu,font=self.font)
		self.ss_choose = tk.IntVar()
		ss_choose = Persist.SUBSIZE
		for ss in range(2,7):
			self.sizemenu.add_radiobutton(label=str(ss*ss)+"x"+str(ss*ss), value=ss, variable=self.ss_choose, command=self.Size,font=self.font)

		self.optmenu = tk.Menu(self.menu,tearoff=0)
		self.menu.add_cascade(label="Options",menu=self.optmenu,font=self.font)
		self.optmenu.add_checkbutton(label="X pattern",onvalue=True,offvalue=False,variable=self.X,font=self.font,command=self.Option)
		self.optmenu.add_checkbutton(label="Window pane",onvalue=True,offvalue=False,variable=self.Window,font=self.font,command=self.Option)

		self.helpmenu = tk.Menu(self.menu,tearoff=0)
		self.menu.add_cascade(label="Help",menu=self.helpmenu,font=self.font)
		self.helpmenu.add_command(label="About",command=self.about,font=self.font)

		self.master.config(menu=self.menu)

def Libs():
	# returns a dict
	s_lib={}
	
	for ss in range(2,7):
		# Shared C library
		lib_base = "./" #location
		lib_base += "sudoku_lib" # base name
		
		lib_base += str(ss*ss)	

		# get the right filename
		if platform.uname()[0] == "Windows":
			lib_base += ".dll" 
		if platform.uname()[0] == "Linux":
			lib_base += ".so" 
		else:
			lib_base += ".dylib" 

		# load library
		global solve_lib
		s_lib[ss] = ctypes.cdll.LoadLibrary(lib_base)
	
	return s_lib

def main(args):
	# keyboard interrupt
	signal.signal(signal.SIGINT, signal_handler)

	# set up library dist
	global solve_lib
	s_lib = Libs()
	 
	while True:
		# load library
		solve_lib = s_lib[Persist.SUBSIZE]
		Sudoku(master=tk.Tk()).mainloop()

if __name__ == "__main__":
	# execute only if run as a script
	sys.exit(main(sys.argv))
